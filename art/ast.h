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
    VECTOR,
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
class call_arguments
{
public:
    virtual ~call_arguments () = default;

    virtual size_t size () const = 0;
    virtual const ast_node& operator [] (size_t index) const = 0;
};


///////////////////////////////
class ast_node
{
public:
    using ptr = std::unique_ptr<ast_node>;

    ast_node () = default;
    virtual ~ast_node () = default;
    virtual std::string to_string () const = 0;
    virtual node_type_enum type () const = 0;
    virtual ast_node::ptr clone () const = 0;

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
    ast_node::ptr clone () const override
    {
        return ast_node::ptr (new ast_node_atom_symbol (m_symbol));
    }

    const std::string& symbol () const
    {
        return m_symbol;
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
    ast_node::ptr clone () const override
    {
        return ast_node::ptr (new ast_node_atom_int (m_value));
    }

    int value () const
    {
        return m_value;
    }

private:
    int m_value;
};

///////////////////////////////
class ast_node_container_base : public ast_node
{
public:
    size_t size () const
    {
        return m_children.size ();
    }

    void add_child (ast_node::ptr child)
    {
        m_children.push_back (std::move (child));
    }

    const ast_node* operator [] (size_t index) const
    {
        assert (index < size ());
        return m_children[index].get ();
    }

    template <typename Fn>
    void map (const Fn& fn)
    {
        for (auto && v : m_children)
        {
            auto newV = fn (std::move (v));
            v = std::move (newV);
        }
    }

protected:
    std::vector<ast_node::ptr> m_children;
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

    ast_node::ptr clone () const override
    {
        auto new_list = std::make_unique<derived> ();
        for (auto &&v : m_children)
        {
            new_list->m_children.push_back (v->clone ());
        }
        return ast_node::ptr (new_list.release ());
    }
};

///////////////////////////////
class ast_node_list : public ast_node_container_crtp <node_type_enum::LIST, ast_node_list>
{
public:
    std::string to_string () const override;

    // FIXME - do we need it?
    ast_node::ptr clear_and_grab_first_child ();

    ///////////////////////////////
    class list_call_arguments : public call_arguments
    {
    public:
        list_call_arguments (const ast_node_list* owner, size_t offset, size_t count)
            : m_owner (owner)
            , m_offset (offset)
            , m_count (count)
        {}
        size_t size () const override
        {
            return m_count;
        }

        const ast_node& operator [] (size_t index) const override
        {
            return *(*m_owner) [index + m_offset];
        }

    private:
        const ast_node_list* m_owner;
        size_t m_offset;
        size_t m_count;
    };
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
    virtual ast_node::ptr call (const call_arguments&) const = 0;
};

///////////////////////////////
class ast_node_callable_builtin : public ast_node_callable
{
public:
    using builtin_fn = ast_node::ptr (*) (const call_arguments&);
    ast_node_callable_builtin (std::string signature, builtin_fn fn);

    std::string to_string () const override
    {
        return m_signature;
    }
    ast_node::ptr clone () const override
    {
        return ast_node::ptr (new ast_node_callable_builtin (m_signature, m_fn));
    }

    ast_node::ptr call (const call_arguments& args) const override;

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

    ast_builder& open_vector ();
    ast_builder& close_vector ();

    ast_builder& add_symbol (std::string);
    ast_builder& add_int (int);

    ast build();

private:
    std::unique_ptr<ast_node_list> m_meta_root;
    std::vector<ast_node_container_base*> m_current_stack;
};
