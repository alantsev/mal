#pragma once

#include "pointer.h"
#include "arena.h"
#include "exceptions.h"

#include <string>
#include <vector>

#include <assert.h>

///////////////////////////////
enum class node_type_enum
{
    UNKNOWN = 0,
    SYMBOL,
    STRING,
    INT,
    LIST,
    CALLABLE,
    NODE_TYPE_COUNT
};

///////////////////////////////
class ast_node;

class ast_node_atom_symbol;
class ast_node_atom_int;

class ast_node_list;

class ast_node_callable;
class ast_node_callable_builtin;

///////////////////////////////
class ast_node
{
public:
    using ptr = std::unique_ptr<ast_node>;

    ast_node () = default;
    virtual ~ast_node () = default;
    virtual std::string to_string () const = 0;
    virtual node_type_enum type () const = 0;

    //
    template <typename T>
    const T* as () const
    {
        assert (type () == T::GET_TYPE ());
        return static_cast<const T*> (this);
    }

    template <typename T, typename TException>
    const T* as_or_throw () const
    {
        if (type () != T::GET_TYPE ())
            raise<TException> ();
        return static_cast<const T*> (this);
    }

    template <typename T, typename TException>
    const T* as_or_zero () const
    {
        if (type () != T::GET_TYPE ())
            return nullptr;
        return static_cast<const T*> (this);
    }

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

private:
    int m_value;
};

///////////////////////////////
class ast_node_list : public ast_node_base <node_type_enum::LIST>
{
public:
    std::string to_string () const override;

    // FIXME - do we need it?
    ast_node::ptr clear_and_grab_first_child ();
    size_t size () const;
    void add_child (ast_node::ptr child);
private:
    std::vector<ast_node::ptr> m_children;
};

///////////////////////////////
class ast_node_callable : public ast_node_base  <node_type_enum::CALLABLE>
{
public:
    class arguments
    {
    public:
        virtual ~arguments () = default;

        virtual size_t size () const = 0;
        virtual const ast_node& operator [] (size_t index) const = 0;
    };

    virtual ast_node::ptr call (const arguments&) const = 0;
};

///////////////////////////////
class ast_node_callable_builtin : public ast_node_callable
{
public:
    using builtin_fn = ast_node::ptr (*) (const ast_node_callable::arguments&);
    ast_node_callable_builtin (std::string signature, builtin_fn fn);

    std::string to_string () const override
    {
        return m_signature;
    }


    ast_node::ptr call (const arguments& args) const override;

private:
    std::string m_signature;
    builtin_fn m_fn;
};

///////////////////////////////
struct ast
{
    ast_node::ptr m_root;
};

///////////////////////////////
class ast_builder
{
public:
    ast_builder ();

    ast_builder& open_list ();
    ast_builder& close_list ();

    ast_builder& add_symbol (std::string);
    ast_builder& add_int (int);

    ast build();

private:
    std::unique_ptr<ast_node_list> m_meta_root;
    std::vector<ast_node_list*> m_current_stack;
};
