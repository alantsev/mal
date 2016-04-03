#pragma once

#include "MAL.h"
#include "ast.h"
#include "environment.h"
#include "exceptions.h"

#include <vector>
#include <deque>
#include <functional>
#include <string>

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
class ast_node_atom : public ast_node_base<node_type_enum::ATOM>
{
public:
  ast_node_atom (ast_node::ptr value) : m_value (value) {}
  std::string to_string (bool print_readable) const override
  {
    return "(atom " + m_value->to_string (print_readable) + ")";
  }

  bool operator == (const ast_node& rp) const override
  {
    return this == std::addressof (rp);
  }

  // actually it's non-const method
  void set_value (ast_node::ptr value) const
  {
    m_value = value;
  }

  ast_node::ptr get_value () const
  {
    return m_value;
  }

  uint32_t hash () const override
  {
    const uint32_t retVal = reinterpret_cast<uint64_t> (this) * 2052828881 + 541325663;
    return retVal;
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_atom> (m_value);
  }

private:
  mutable ast_node::ptr m_value;
};

///////////////////////////////
class ast_node_symbol : public ast_node_base <node_type_enum::SYMBOL>
{
public:
  ast_node_symbol (std::string a_symbol);
  std::string to_string (bool print_readable) const override;

  const std::string& symbol () const
  {
    return m_symbol;
  }

  bool operator == (const ast_node& rp) const override
  {
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

    return m_symbol == rp.as<ast_node_symbol> ()->m_symbol;
  }

  uint32_t hash () const override
  {
    return std::hash<std::string> () (m_symbol);
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_symbol> (m_symbol);
  }

private:
  std::string m_symbol;
};

///////////////////////////////
class ast_node_string : public ast_node_base <node_type_enum::STRING>
{
public:
  ast_node_string (std::string val)
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
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

    return m_value == rp.as<ast_node_string> ()->m_value;
  }

  uint32_t hash () const override
  {
    return std::hash<std::string> () (m_value) * 547449787 + 1550872369;
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_string> (m_value);
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
class ast_node_keyword : public ast_node_base <node_type_enum::KEYWORD>
{
public:
  ast_node_keyword (std::string val)
    : m_keyword (std::move (val))
  {}

  std::string to_string (bool print_readable) const override
  {
    UNUSED (print_readable);
    return m_keyword;
  }

  const std::string& keyword () const
  {
    return m_keyword;
  }

  bool operator == (const ast_node& rp) const override
  {
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

    return m_keyword == rp.as<ast_node_keyword> ()->m_keyword;
  }

  uint32_t hash () const override
  {
    return std::hash<std::string> () (m_keyword) * 1965751841 + 311457019;
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_keyword> (m_keyword);
  }

private:
  //
  std::string m_keyword;
};

///////////////////////////////
class ast_node_int : public ast_node_base <node_type_enum::INT>
{
public:
  ast_node_int (int64_t a_value);

  std::string to_string (bool print_readable) const override;
  int64_t value () const
  {
    return m_value;
  }

  bool operator == (const ast_node& rp) const override
  {
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

    return m_value == rp.as<ast_node_int> ()->m_value;
  }

  uint32_t hash () const override
  {
    return std::hash<int> () (m_value);
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_int> (m_value);
  }

private:
  int64_t m_value;
};

///////////////////////////////
template <bool VALUE>
class ast_node_bool : public ast_node_base <node_type_enum::BOOL>
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
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

    return this == &rp;
  }

  uint32_t hash () const override
  {
    return std::hash<bool> () (VALUE);
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_bool<VALUE> > ();
  }

};

///////////////////////////////
class ast_node_nil : public ast_node_base <node_type_enum::NIL>
{
public:
  ast_node_nil () = default;
  std::string to_string (bool print_readable) const override;

  bool operator == (const ast_node& rp) const override
  {
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

    return this == &rp;
  }

  uint32_t hash () const override
  {
    return std::hash<uint32_t> () (1);
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_nil> ();
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

  uint32_t hash () const override
  {
    uint32_t retVal = 1722983309;
    for (auto && p : m_children)
    {
      retVal = (retVal + p->hash ()) * 824928359 + 1722983309;
    }
    return retVal;
  }


  bool operator == (const ast_node& rp) const override
  {
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

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
class ast_node_ht_list : public ast_node_container_crtp <node_type_enum::HT_LIST, ast_node_ht_list>
{
public:
  std::string to_string (bool print_readable) const override
  {
    std::string retVal = "{";
    for (size_t i = 0, e = m_children.size (); i < e; ++i)
    {
      auto &&p = m_children [i];
      if (i != 0)
        retVal += " ";
      retVal += p->to_string (print_readable);
    }
    retVal += "}";

    return retVal;
  }

  uint32_t hash () const override
  {
    uint32_t retVal = 1003322101;
    for (auto && p : m_children)
    {
      retVal = (retVal + p->hash ()) * 291675463 + 1003322101;
    }
    return retVal;
  }

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return node_type_enum::HT_LIST == t;
  }

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
    return (t == node_type_enum::CALLABLE_BUILTIN) || (t == node_type_enum::CALLABLE_LAMBDA) || (t == node_type_enum::HASHMAP);
  }
};


///////////////////////////////
class ast_node_callable_builtin_base : public ast_node_callable
{
public:
  bool operator == (const ast_node& rp) const override
  {
    if (type() != rp.type ())
      return false;

    return signature () == rp.as<ast_node_callable_builtin_base> ()->signature ();
  }

  uint32_t hash () const override
  {
    return std::hash<std::string> () (m_signature) * 769451167 + 267085321;
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

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return node_type_enum::CALLABLE_BUILTIN == t;
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_callable_builtin<builtin_fn> > (signature (), m_fn);
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
    if (type () != rp.type ())
      return false;

    auto rp_lambda = rp.as<ast_node_callable_lambda> ();
    return equals (*m_binds, *rp_lambda->m_binds) && equals (*m_ast, *rp_lambda->m_ast);
  }

  node_type_enum type () const override
  {
    return node_type_enum::CALLABLE_LAMBDA;
  }

  uint32_t hash () const override
  {
    return (m_binds->hash () * 1622000167 + 582512737) * m_ast->hash () + 2152752083;
  }

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return node_type_enum::CALLABLE_LAMBDA == t;
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_callable_lambda> (m_binds, m_ast, m_outer_env);
  }

private:
  ast_node::ptr m_binds;
  const ast_node_container_base* m_binds_as_container;

  ast_node::ptr m_ast;
  environment::const_ptr m_outer_env;
};

///////////////////////////////
class ast_node_macro_call : public ast_node_base <node_type_enum::MACRO_CALL>
{
public:
  ast_node_macro_call (ast_node::ptr node)
    : m_callable_node (node)
  {
    m_callable_node->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();
  }

  std::string to_string (bool print_readable) const override
  {
    return "#macro-call-" + m_callable_node->to_string ();
  }

  bool operator == (const ast_node& rp) const override
  {
    if (!IS_VALID_TYPE (rp.type ()))
      return false;

    return *m_callable_node == *rp.as<ast_node_macro_call> ()->m_callable_node;
  }

  uint32_t hash () const override
  {
    return m_callable_node->hash ();
  }

  ast_node::ptr callable_node () const 
  {
    return m_callable_node;
  }

protected:
  mutable_ptr clone () const override
  {
    return std::make_shared<ast_node_macro_call> (m_callable_node);
  }

private:
  //
  ast_node::ptr m_callable_node;
};

///////////////////////////////
class ast_node_hashmap : public ast_node_callable
{
public:
  ast_node_hashmap () {}

  std::string to_string (bool print_readable) const override
  {
    std::string retVal = "{";
    size_t i = 0;
    for (auto && p : m_hashtable)
    {
      if (i != 0)
        retVal += " ";
      retVal += p.first->to_string (print_readable) + " " + p.second->to_string (print_readable);
      ++i;
    }
    retVal += "}";

    return retVal;
  }

  tco call_tco (const call_arguments &args) const override
  {
    // TODO - 
    if (args.size () != 1)
      raise<mal_exception_eval_invalid_arg> ("");

    auto it = m_hashtable.find (args [0]);
    return tco{nullptr, nullptr, it != m_hashtable.end () ? it->second : ast_node::nil_node};
  }

  bool operator == (const ast_node& rp) const override
  {
    if (type () != rp.type ())
      return false;

    auto rp_hashmap = rp.as<ast_node_hashmap> ();

    if (m_hashtable.size () != rp_hashmap->m_hashtable.size ())
      return false;

    for (auto it1 = m_hashtable.begin(), e = m_hashtable.end (); it1 != e; ++it1)
    {
      auto it2 = rp_hashmap->m_hashtable.find (it1->first);
      if (it2 == rp_hashmap->m_hashtable.end ())
        return false;

      if (*it1->second != *it2->second)
        return false;
    }
    return true;
  }

  node_type_enum type () const override
  {
    return node_type_enum::HASHMAP;
  }

  uint32_t hash () const override
  {
    uint32_t retVal = 582512737;
    for (auto && p : m_hashtable)
    {
      retVal = (retVal + p.first->hash ()) * 1622000167 + 2152752083;
      retVal = (retVal + p.second->hash ()) * 1622000167 + 2152752083;
    }
    return retVal;
  }

  void insert (ast_node::ptr key, ast_node::ptr value)
  {
    m_hashtable [key] = value;
  }

  void erase (ast_node::ptr key)
  {
    m_hashtable.erase (key);
  }

  ast_node::ptr get (ast_node::ptr key) const
  {
    auto it = m_hashtable.find (key);
    return it != m_hashtable.end () ? it->second : ast_node::nil_node;
  }

  bool has (ast_node::ptr key) const
  {
    return m_hashtable.count (key) != 0;
  }

  static constexpr bool IS_VALID_TYPE (node_type_enum t)
  {
    return node_type_enum::HASHMAP == t;
  }

  mutable_ptr clone () const override
  {
    auto retVal = std::make_shared<ast_node_hashmap> ();
    retVal->m_hashtable = m_hashtable;
    return retVal;
  }

  template <typename Visitor>
  void for_each (Visitor && v) const
  {
    for (auto && p : m_hashtable)
    {
      v (p.first, p.second);
    }
  }

private:
  struct ast_node_hash
  {
    uint32_t operator () (ast_node::ptr v) const
    {
      return v ? v->hash () : 0;
    }
  };

  struct ast_node_eq
  {
    bool operator () (ast_node::ptr v1, ast_node::ptr v2) const
    {
      return v1 == v2 ? true :
             v1 && v2 ? *v1 == *v2 : false;
    }

  };

  std::unordered_map <ast_node::ptr, ast_node::ptr, ast_node_hash, ast_node_eq> m_hashtable;
};

namespace mal
{
  ///////////////////////////////
  inline std::shared_ptr<ast_node_list> 
  make_list () 
  {
    return std::make_shared<ast_node_list> ();
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_vector> 
  make_vector () 
  {
    return std::make_shared<ast_node_vector> ();
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_ht_list> 
  make_ht_list () 
  {
    return std::make_shared<ast_node_ht_list> ();
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_hashmap>
  make_hashmap (const ast_node_container_base* seq)
  {
    auto retVal = std::make_shared<ast_node_hashmap> ();

    const size_t count = seq->size ();
    if (count % 2 != 0)
      raise<mal_exception_parse_error> ("odd number of elements in hashmap");

    for (size_t i = 0; i < count; i += 2)
    {
      retVal->insert ((*seq)[i], (*seq)[i + 1]);
    }

    return retVal;
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_hashmap>
  make_hashmap (const call_arguments& args)
  {
    auto retVal = std::make_shared<ast_node_hashmap> ();

    const size_t count = args.size ();
    if (count % 2 != 0)
      raise<mal_exception_parse_error> ("odd number of elements in hashmap");

    for (size_t i = 0; i < count; i += 2)
    {
      retVal->insert (args[i], args[i + 1]);
    }

    return retVal;
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_symbol> 
  make_symbol (std::string value) 
  {
    return std::make_shared<ast_node_symbol> (std::move (value));
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_keyword> 
  make_keyword (std::string value) 
  {
    return std::make_shared<ast_node_keyword> (std::move (value));
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_string> 
  make_string (std::string value) 
  {
    return std::make_shared<ast_node_string> (std::move (value));
  }

  ///////////////////////////////
  inline std::shared_ptr<ast_node_int> 
  make_int (int64_t value) 
  {
    return std::make_shared<ast_node_int> (value);
  }
}


