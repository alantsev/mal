#pragma once

#include "MAL.h"
#include "ast.h"
#include "environment.h"

///////////////////////////////
template <node_type_enum NODE_TYPE>
class ast_node_base : public ast_node
{
public:
  node_type_enum type () const override
  {
    return NODE_TYPE;
  }

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return NODE_TYPE == t;
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
  std::string to_string (bool print_readable) const override;

  const std::string& symbol () const
  {
    return m_symbol;
  }

  bool operator == (const ast_node& rp) const override
  {
    return m_symbol == rp.as<ast_node_atom_symbol> ()->m_symbol;
  }

private:
  std::string m_symbol;
};

///////////////////////////////
class ast_node_atom_string : public ast_node_atom <node_type_enum::STRING>
{
public:
  ast_node_atom_string (std::string val)
    : m_value (std::move (val))
  {}

  std::string to_string (bool print_readable) const override
  {
    if (!print_readable)
      return m_value;

    std::string retVal = m_value;
    replace_all (retVal, "\\", "\\\\");
    replace_all (retVal, "\n", "\\n");
    replace_all (retVal, "\"", "\\\"");
    return "\"" + retVal + "\"";
  }

  const std::string& value () const
  {
    return m_value;
  }

  bool operator == (const ast_node& rp) const override
  {
    return m_value == rp.as<ast_node_atom_string> ()->m_value;
  }

private:
  //
  void replace_all(std::string &str, const std::string &from, const std::string &to) const
  {
    size_t start_pos = 0;
    while ((start_pos = str.find (from, start_pos)) != std::string::npos) 
    {
      str.replace (start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
  }

  std::string m_value;
};

///////////////////////////////
class ast_node_atom_int : public ast_node_atom <node_type_enum::INT>
{
public:
  ast_node_atom_int (int a_value);

  std::string to_string (bool print_readable) const override;
  int value () const
  {
    return m_value;
  }

  bool operator == (const ast_node& rp) const override
  {
    return m_value == rp.as<ast_node_atom_int> ()->m_value;
  }

private:
  int m_value;
};

///////////////////////////////
template <bool VALUE>
class ast_node_atom_bool : public ast_node_atom <node_type_enum::BOOL>
{
public:
  std::string to_string (bool print_readable) const override
  {
    return VALUE ? "true" : "false";
  }

  bool value () const
  {
    return VALUE;
  }
  explicit operator bool () const
  {
    return VALUE;
  }

  bool operator == (const ast_node& rp) const override
  {
    return this == &rp;
  }

};

///////////////////////////////
class ast_node_atom_nil : public ast_node_atom <node_type_enum::NIL>
{
public:
  ast_node_atom_nil () = default;
  std::string to_string (bool print_readable) const override;

  bool operator == (const ast_node& rp) const override
  {
    return this == &rp;
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
    m_children.push_back (child);
  }

  ast_node::ptr operator [] (size_t index) const
  {
    assert (index < size ());
    return m_children[index];
  }

  template <typename Fn>
  ast_node::ptr map (const Fn& fn) const
  {
    mutable_ptr retVal = this->clone ();

    auto ptr = static_cast<ast_node_container_base*> (retVal.get ());
    ptr->map_impl (fn);

    return retVal;
  }

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return (t == node_type_enum::LIST) || (t == node_type_enum::VECTOR);
  }

  bool operator == (const ast_node& rp) const override
  {
    assert (type () == rp.type ());

    auto rp_container = rp.as<ast_node_container_base> ();
    if (size () != rp_container->size())
      return false;

    bool retVal = true;
    for (size_t i = 0, e = size (); i < e && retVal; ++i)
    {
      retVal = retVal && equals (*(m_children[i]), *(rp_container->m_children[i]));
    }

    return retVal;
  }

protected:
  using mutable_ptr = std::shared_ptr<ast_node>;
  virtual mutable_ptr clone () const = 0;

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

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return NODE_TYPE == t;
  }

protected:
  mutable_ptr clone () const override
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
  using ast_node::to_string;
  std::string to_string (bool print_readable) const override;
};

///////////////////////////////
class ast_node_vector : public ast_node_container_crtp <node_type_enum::VECTOR, ast_node_vector>
{
public:
  std::string to_string (bool print_readably) const override;
};

///////////////////////////////
// tree, env, retVal
using tco = std::tuple <ast, environment::ptr, ast>;

///////////////////////////////
class ast_node_callable : public ast_node
{
public:
  virtual tco call_tco (const call_arguments&) const = 0;

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return (t == node_type_enum::CALLABLE_BUILTIN) || (t == node_type_enum::CALLABLE_LAMBDA);
  }
};


///////////////////////////////
class ast_node_callable_builtin_base : public ast_node_callable
{
public:
  bool operator == (const ast_node& rp) const override
  {
    return signature () == rp.as<ast_node_callable_builtin_base> ()->signature ();
  }

protected:
  ast_node_callable_builtin_base (std::string signature)
    : m_signature (std::move (signature))
  {}

  const std::string& signature () const
  {
    return m_signature;
  }

private:
  std::string m_signature;
};

///////////////////////////////
//   using builtin_fn = ast_node::ptr (*) (const call_arguments&);
template <typename builtin_fn>
class ast_node_callable_builtin : public ast_node_callable_builtin_base
{
public:
  ast_node_callable_builtin (std::string signature, builtin_fn fn)
    : ast_node_callable_builtin_base (std::move (signature))
    , m_fn (fn)
  {}

  std::string to_string (bool print_readable) const override
  {
    return std::string ("#builtin-fn(") + signature () + ")";
  }

  tco call_tco (const call_arguments &args) const override
  {
    return tco{nullptr, nullptr, m_fn (args)};
  }

  node_type_enum type () const override
  {
    return node_type_enum::CALLABLE_BUILTIN;
  }

private:
  builtin_fn m_fn;
};

///////////////////////////////
class ast_node_callable_lambda : public ast_node_callable
{
public:
  ast_node_callable_lambda (ast_node::ptr binds, ast_node::ptr ast, environment::const_ptr outer_env);

  std::string to_string (bool print_readable) const override
  {
//    return "#callable-lambda" ;
    return "#callable-lambda" + m_binds->to_string ()+ " -> " + m_ast->to_string ();
  }

  tco call_tco (const call_arguments&) const override;

  bool operator == (const ast_node& rp) const override
  {
    auto rp_lambda = rp.as<ast_node_callable_lambda> ();
    return equals (*m_binds, *rp_lambda->m_binds) && equals (*m_ast, *rp_lambda->m_ast);
  }

  node_type_enum type () const override
  {
    return node_type_enum::CALLABLE_LAMBDA;
  }

private:
  ast_node::ptr m_binds;
  const ast_node_container_base* m_binds_as_container;

  ast_node::ptr m_ast;
  environment::const_ptr m_outer_env;
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

  // adds complete string
  ast_builder& add_string (std::string);

  // builds string by pieces
  ast_builder& start_string ();
  ast_builder& append_string (const std::string &piece);
  ast_builder& finish_string ();

  ast build();

private:
  std::unique_ptr<ast_node_list> m_meta_root;
  std::vector<ast_node_container_base*> m_current_stack;
  std::string m_picewise_string;
};

