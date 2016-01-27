#pragma once

#include "pointer.h"
#include "arena.h"
#include "exceptions.h"

#include <string>
#include <vector>
#include <functional>

#include <assert.h>

///////////////////////////////
class environment;

///////////////////////////////
enum class node_type_enum
{
  UNKNOWN = 0,
  SYMBOL,
  STRING,
  INT,
  BOOL,
  NIL,
  LIST,
  VECTOR,
  CALLABLE,
  NODE_TYPE_COUNT
};

///////////////////////////////
class ast_node;

class ast_node_atom_symbol;
class ast_node_atom_int;

class ast_node_container_base;
class ast_node_list;
class ast_node_vector;

class ast_node_callable;
class ast_node_callable_builtin;
class ast_node_callable_lambda;

///////////////////////////////
class ast_node
{
public:
  using ptr = std::shared_ptr<const ast_node>;

  static ptr nil_node;
  static ptr true_node;
  static ptr false_node;

  //
  ast_node () = default;
  virtual ~ast_node () = default;
  virtual std::string to_string () const = 0;
  virtual node_type_enum type () const = 0;

  //
  template <typename T>
  T* as ()
  {
    assert (type () == T::GET_TYPE ());
    return static_cast<T*> (this);
  }
  template <typename T>
  const T* as () const
  {
    assert (type () == T::GET_TYPE ());
    return static_cast<const T*> (this);
  }

  template <typename T, typename TException>
  T* as_or_throw ()
  {
    if (type () != T::GET_TYPE ())
      raise<TException> (this->to_string ());
    return static_cast<T*> (this);
  }
  template <typename T, typename TException>
  const T* as_or_throw () const
  {
    if (type () != T::GET_TYPE ())
      raise<TException> (this->to_string ());
    return static_cast<const T*> (this);
  }

  template <typename T, typename TException>
  T* as_or_zero ()
  {
    if (type () != T::GET_TYPE ())
      return nullptr;
    return static_cast<T*> (this);
  }
  template <typename T, typename TException>
  const T* as_or_zero () const
  {
    if (type () != T::GET_TYPE ())
      return nullptr;
    return static_cast<const T*> (this);
  }

protected:
  using mutable_ptr = std::shared_ptr<ast_node>;
  virtual ast_node::mutable_ptr clone () const = 0;

private:
  ast_node (const ast_node&) = delete;
  ast_node& operator = (const ast_node&) = delete;
};

///////////////////////////////
template <node_type_enum NODE_TYPE>
class ast_node_base : public ast_node
{
public:
  node_type_enum type () const override
  {
    return NODE_TYPE;
  }

  static constexpr node_type_enum GET_TYPE ()
  {
    return NODE_TYPE;
  }
};

///////////////////////////////
template <node_type_enum NODE_TYPE>
class ast_node_atom : public ast_node_base<NODE_TYPE>
{};

///////////////////////////////
class ast_node_atom_symbol : public ast_node_atom <node_type_enum::SYMBOL>
{
public:
  ast_node_atom_symbol (std::string a_symbol);
  std::string to_string () const override;

  const std::string& symbol () const
  {
    return m_symbol;
  }

protected:
  ast_node::mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_atom_symbol> (m_symbol);
  }

private:
  std::string m_symbol;
};

///////////////////////////////
class ast_node_atom_int : public ast_node_atom <node_type_enum::INT>
{
public:
  ast_node_atom_int (int a_value);

  std::string to_string () const override;
  int value () const
  {
    return m_value;
  }

protected:
  ast_node::mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_atom_int> (m_value);
  }

private:
  int m_value;
};

///////////////////////////////
class ast_node_atom_bool : public ast_node_atom <node_type_enum::BOOL>
{
public:
  ast_node_atom_bool (bool value);

  std::string to_string () const override;
  bool value () const
  {
    return m_value;
  }
  explicit operator bool () const
  {
    return m_value;
  }

protected:
  ast_node::mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_atom_bool> (m_value);
  }

private:
  bool m_value;
};

///////////////////////////////
class ast_node_atom_nil : public ast_node_atom <node_type_enum::NIL>
{
public:
  ast_node_atom_nil () = default;
  std::string to_string () const override;

protected:
  ast_node::mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_atom_nil> ();
  }
};

///////////////////////////////
class ast_node_container_base : public ast_node
{
public:
  size_t size () const
  {
    return m_children.size ();
  }

  bool empty () const
  {
    return size () == 0;
  }

  void add_child (ast_node::ptr child)
  {
    m_children.push_back (std::move (child));
  }

  ast_node::ptr operator [] (size_t index) const
  {
    assert (index < size ());
    return m_children[index];
  }

  template <typename Fn>
  ast_node::ptr map (const Fn& fn) const
  {
    ast_node::mutable_ptr retVal = this->clone ();

    auto ptr = static_cast<ast_node_container_base*> (retVal.get ());
    ptr->map_impl (fn);

    return retVal;
  }

protected:
  std::vector<ast_node::ptr> m_children;

private:
  template <typename Fn>
  void map_impl (const Fn& fn)
  {
    for (auto && v : m_children)
    {
      v = fn (v);
    }
  }
};

///////////////////////////////
class call_arguments
{
public:
  call_arguments (const ast_node_container_base* owner, size_t offset, size_t count)
    : m_owner (owner)
    , m_offset (offset)
    , m_count (count)
  {}

  size_t size () const
  {
    return m_count;
  }

  ast_node::ptr operator [] (size_t index) const
  {
    return (*m_owner) [index + m_offset];
  }

private:
  const ast_node_container_base* m_owner;
  size_t m_offset;
  size_t m_count;
};

///////////////////////////////
template <node_type_enum NODE_TYPE, typename derived>
class ast_node_container_crtp : public ast_node_container_base
{
public:
  node_type_enum type () const override
  {
    return NODE_TYPE;
  }

  static constexpr node_type_enum GET_TYPE ()
  {
    return NODE_TYPE;
  }

protected:
  ast_node::mutable_ptr clone () const override
  {
    auto new_list = std::make_shared<derived> ();
    for (auto &&v : m_children)
    {
      new_list->m_children.push_back (v);
    }
    return new_list;
  }

};

///////////////////////////////
class ast_node_list : public ast_node_container_crtp <node_type_enum::LIST, ast_node_list>
{
public:
  std::string to_string () const override;

  // FIXME - do we need it?
  ast_node::ptr clear_and_grab_first_child ();
};

///////////////////////////////
class ast_node_vector : public ast_node_container_crtp <node_type_enum::VECTOR, ast_node_vector>
{
public:
  std::string to_string () const override;
};

///////////////////////////////
class ast_node_callable : public ast_node_base  <node_type_enum::CALLABLE>
{
public:
  virtual ast_node::ptr call (const call_arguments&, const environment& outer_env) const = 0;
};

///////////////////////////////
class ast_node_callable_builtin : public ast_node_callable
{
public:
  using builtin_fn = ast_node::ptr (*) (const call_arguments&);
  ast_node_callable_builtin (const std::string &signature, builtin_fn fn);

  std::string to_string () const override
  {
    return m_signature ;
  }

  ast_node::ptr call (const call_arguments& args, const environment&) const override;

protected:
  ast_node::mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_callable_builtin> (m_signature, m_fn);
  }

private:
  std::string m_signature;
  builtin_fn m_fn;
};

///////////////////////////////
class ast_node_callable_lambda : public ast_node_callable
{
public:
  using eval_fn = std::function<ast_node::ptr (ast_node::ptr, environment&)>;
  ast_node_callable_lambda (ast_node::ptr binds, ast_node::ptr ast, eval_fn eval);

  std::string to_string () const override
  {
    return "#callable-lambda" ;
  }

  ast_node::ptr call (const call_arguments& args, const environment& outer_env) const override;

protected:
  ast_node::mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_callable_lambda> (m_binds, m_ast, m_eval);
  }

private:
  ast_node::ptr m_binds;
  const ast_node_container_base* m_binds_as_container;

  ast_node::ptr m_ast;
  eval_fn m_eval;
};

///////////////////////////////
class ast
{
public:
  // implicit by intention
  ast (ast_node::ptr node = nullptr)
    : m_node (node)
  {}

  ast_node::ptr operator -> () const
  {
    return m_node;
  }

  explicit operator bool () const
  {
    return !!m_node;
  }

  operator ast_node::ptr () const
  {
    return m_node;
  }

private:
  ast_node::ptr m_node;
};

///////////////////////////////
class ast_builder
{
public:
  ast_builder ();

  ast_builder& open_list ();
  ast_builder& close_list ();

  ast_builder& open_vector ();
  ast_builder& close_vector ();

  ast_builder& add_symbol (std::string);
  ast_builder& add_int (int);

  ast_builder& add_bool (bool val);
  ast_builder& add_nil ();

  ast build();

private:
  std::unique_ptr<ast_node_list> m_meta_root;
  std::vector<ast_node_container_base*> m_current_stack;
};
