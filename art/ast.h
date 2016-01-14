#pragma once

#include "pointer.h"
#include "arena.h"

#include <string>
#include <vector>

#include <assert.h>

///////////////////////////////
class ast_node
{
public:
    virtual ~ast_node () = default;

    virtual std::string to_string () const = 0;
};

///////////////////////////////
class ast_node_atom : public ast_node
{
};

///////////////////////////////
class ast_atom_symbol : public ast_node_atom
{
public:
    ast_atom_symbol (std::string a_symbol);

    std::string to_string () const override;

private:
    std::string m_symbol;
};

///////////////////////////////
class ast_atom_int : public ast_node_atom
{
public:
    ast_atom_int (int a_value);

    std::string to_string () const override;

private:
    int m_value;
};

///////////////////////////////
class ast_node_list : public ast_node
{
public:
    std::string to_string () const override;

    std::unique_ptr<ast_node> clear_and_grab_first_child ();
    size_t size () const;
    void add_children (std::unique_ptr<ast_node> child);

private:
    std::vector<std::unique_ptr<ast_node> > m_children;
};

///////////////////////////////
struct ast
{
    std::unique_ptr<ast_node> m_root;
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
