#include "MAL.h"
#include "ast.h"

///////////////////////////////
/// ast_node_list class
///////////////////////////////
std::string 
ast_node_list::to_string () const // override
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

///////////////////////////////
ast_node::ptr
ast_node_list::clear_and_grab_first_child () 
{
    assert (size () > 0);
    ast_node::ptr retVal = std::move (m_children [0]);
    m_children.clear ();
    return std::move (retVal);
}

///////////////////////////////
size_t 
ast_node_list::size () const
{
    return m_children.size ();
}

///////////////////////////////
void 
ast_node_list::add_child (ast_node::ptr child)
{
    m_children.push_back (std::move (child));
}

///////////////////////////////
/// ast_node_atom_symbol class
///////////////////////////////
ast_node_atom_symbol::ast_node_atom_symbol (std::string a_symbol)
    : m_symbol (std::move (a_symbol))
{}

///////////////////////////////
std::string
ast_node_atom_symbol::to_string () const // override
{
    // FIXME
    return m_symbol;
}

///////////////////////////////
/// ast_node_atom_int class
///////////////////////////////
ast_node_atom_int::ast_node_atom_int (int a_value)
    : m_value (a_value)
{}

///////////////////////////////
std::string
ast_node_atom_int::to_string () const // override
{
    // FIXME
    return std::to_string (m_value);
}

///////////////////////////////
/// ast_node_callable_builtin class
///////////////////////////////
ast_node_callable_builtin::ast_node_callable_builtin (std::string signature, ast_node_callable_builtin::builtin_fn fn)
    : m_signature (std::move (signature))
    , m_fn (fn)
{}

///////////////////////////////
ast_node::ptr 
ast_node_callable_builtin::call (const call_arguments& args) const
{
    return m_fn (args);
}

///////////////////////////////
/// ast_builder class
///////////////////////////////
ast_builder::ast_builder ()
    : m_meta_root (new ast_node_list {})
    , m_current_stack ({m_meta_root.get ()})
{}

///////////////////////////////
ast_builder&
ast_builder::open_list ()
{
    std::unique_ptr<ast_node_list> child { new ast_node_list {} };
    ast_node_list* child_ref = child.get ();

    m_current_stack.back ()->add_child (std::move (child));
    m_current_stack.push_back (child_ref);
    return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::close_list ()
{
    m_current_stack.pop_back ();
    if (m_current_stack.size () == 0) 
        raise<mal_exception_parse_error> ();

    return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::add_symbol (std::string value)
{
    std::unique_ptr<ast_node_atom_symbol> child { new ast_node_atom_symbol {std::move (value)} };
    m_current_stack.back ()->add_child (std::move (child));
    return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::add_int (int value)
{
    std::unique_ptr<ast_node_atom_int> child { new ast_node_atom_int { value } };
    m_current_stack.back ()->add_child (std::move (child));
    return *this;
}

///////////////////////////////
ast 
ast_builder::build()
{
    if (m_current_stack.size () != 1) 
        raise<mal_exception_parse_error> ();

    const size_t level0_count = m_current_stack.back()->size (); 
    if (level0_count > 1) 
        raise<mal_exception_parse_error> ();

    if (level0_count == 0)
        return ast {};

    ast retVal {std::move (m_current_stack.back()->clear_and_grab_first_child ())};
    m_current_stack.clear ();
    m_meta_root.reset ();

    return std::move (retVal);
}
