#pragma once

#include "pointer.h"
#include "arena.h"

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
class ast_node
{
public:
    using ptr = std::unique_ptr<ast_node>;

    ast_node () = default;
    virtual ~ast_node () = default;
    virtual std::string to_string () const = 0;
    virtual node_type_enum type () const = 0;

    // FIXME
    // as<int> () -> ???

    // FIXME - addme ?
    // virtual ptr clone () const = 0;

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
};

///////////////////////////////
template <node_type_enum NODE_TYPE>
class ast_node_atom : public ast_node_base<NODE_TYPE>
{};

///////////////////////////////
class ast_atom_symbol : public ast_node_atom <node_type_enum::SYMBOL>
{
public:
    ast_atom_symbol (std::string a_symbol);

    std::string to_string () const override;

private:
    std::string m_symbol;
};

///////////////////////////////
class ast_atom_int : public ast_node_atom <node_type_enum::INT>
{
public:
    ast_atom_int (int a_value);

    std::string to_string () const override;

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
// FIXME - implement me!
//template <>
//class ast_node_callable_builtin :

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
