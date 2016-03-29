#pragma once

#include "pointer.h"
#include "arena.h"
#include "exceptions.h"
#include "MAL.h"

#include <string>
#include <vector>
#include <functional>

#include <assert.h>

///////////////////////////////
enum class node_type_enum
{
  UNKNOWN = 0,
  ATOM,
  SYMBOL,
  KEYWORD,
  STRING,
  INT,
  BOOL,
  NIL,
  LIST,
  VECTOR,
  HASHMAP,
  CALLABLE_BUILTIN,
  CALLABLE_LAMBDA,
  MACRO_CALL,
  HT_LIST, // for internal use only
  NODE_TYPE_COUNT
};

///////////////////////////////
class ast_node
{
public:
  using ptr = std::shared_ptr <const ast_node>;

  static ptr nil_node;
  static ptr true_node;
  static ptr false_node;

  //
  ast_node () = default;
  virtual ~ast_node () = default;

  virtual std::string to_string () const
  {
    return to_string (true);
  }

  virtual std::string to_string (bool print_readable) const = 0;
  virtual node_type_enum type () const = 0;

  virtual bool operator == (const ast_node&) const = 0;
  virtual uint32_t hash () const = 0;


  //
  template <typename T>
  T* as ()
  {
    assert (T::IS_VALID_TYPE (type ()));
    return static_cast<T*> (this);
  }
  template <typename T>
  const T* as () const
  {
    assert (T::IS_VALID_TYPE (type ()));
    return static_cast<const T*> (this);
  }

  template <typename T, typename TException>
  T* as_or_throw ()
  {
    if (!T::IS_VALID_TYPE (type ()))
      raise<TException> (this->to_string ());
    return static_cast<T*> (this);
  }
  template <typename T, typename TException>
  const T* as_or_throw () const
  {
    if (!T::IS_VALID_TYPE (type ()))
      raise<TException> (this->to_string ());
    return static_cast<const T*> (this);
  }

  template <typename T>
  T* as_or_zero ()
  {
    if (!T::IS_VALID_TYPE (type ()))
      return nullptr;
    return static_cast<T*> (this);
  }
  template <typename T>
  const T* as_or_zero () const
  {
    if (!T::IS_VALID_TYPE (type ()))
      return nullptr;
    return static_cast<const T*> (this);
  }

private:
  ast_node (const ast_node&) = delete;
  ast_node& operator = (const ast_node&) = delete;
};

///////////////////////////////
inline bool equals (const ast_node& left, const ast_node& right)
{
  return left == right;
}

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
