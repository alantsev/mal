#pragma once

#include <string>
#include <vector>

#include <assert.h>

///////////////////////////////
class ast_node
{
public:
    ast_node () {}
    virtual ~ast_node () {}

    virtual std::string to_string () const = 0;
};

///////////////////////////////
class ast_node_atom : public ast_node
{
};

///////////////////////////////
class ast_node_list : public ast_node
{
public:

    std::string to_string () const override
    {
        std::string retVal = "(";
        for (size_t i = 0, e = m_children.size (); i < e; ++i)
        {
            auto &&p = m_children [i];
            if (i != 0)
                retVal += " ";
            retVal += p->to_string ();
        }
        retVal += ")";

        return retVal;
    }

    std::unique_ptr<ast_node> clear_and_grab_first_child () 
    {
        assert (size () > 0);
        std::unique_ptr<ast_node> retVal = std::move (m_children [0]);
        m_children.clear ();
        return std::move (retVal);
    }

    size_t size () const
    {
        return m_children.size ();
    }

    void add_children (std::unique_ptr<ast_node> child)
    {
        m_children.push_back (std::move (child));
    }


private:
    std::vector<std::unique_ptr<ast_node> > m_children;
};

///////////////////////////////
class ast_atom_symbol : public ast_node_atom
{
public:
    ast_atom_symbol (std::string a_symbol)
        : m_symbol (std::move (a_symbol))
    {}

    std::string to_string () const override
    {
        // FIXME
        return m_symbol;
    }

private:
    std::string m_symbol;
};

///////////////////////////////
class ast_atom_int : public ast_node_atom
{
public:
    ast_atom_int (int a_value)
        : m_value (a_value)
    {}

    std::string to_string () const override
    {
        // FIXME
        return std::to_string (m_value);
    }

private:
    int m_value;
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
