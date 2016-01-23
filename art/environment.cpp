#include "MAL.h"
#include "environment.h"

#include <iostream>

///////////////////////////////
ast_node::ptr 
builtin_plus (const call_arguments& args)
{
    const auto args_size = args.size ();
    if (args_size < 1)
        raise<mal_exception_eval_invalid_arg> ();

    int retVal = 0;
    for (size_t i = 0; i < args_size; ++i)
        retVal += args[i].as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();

    return std::make_unique<ast_node_atom_int> (retVal);
}

///////////////////////////////
ast_node::ptr 
builtin_minus (const call_arguments& args)
{
    const auto args_size = args.size ();
    if (args_size < 1 || args_size > 2)
        raise<mal_exception_eval_invalid_arg> ();

    auto arg_to_int = [&args](size_t i) -> int
    {
        return args[i].as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();
    };

    int retVal = (args_size == 1) ? -arg_to_int (0) : arg_to_int (0) - arg_to_int (1);
    return std::make_unique<ast_node_atom_int> (retVal);
}

///////////////////////////////
ast_node::ptr 
builtin_div (const call_arguments& args)
{
    const auto args_size = args.size ();
    if (args_size != 2)
        raise<mal_exception_eval_invalid_arg> ();

    auto arg_to_int = [&args](size_t i) -> int
    {
        return args[i].as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();
    };

    int retVal = arg_to_int(0) / arg_to_int(1);
    return std::make_unique<ast_node_atom_int> (retVal);
}

///////////////////////////////
ast_node::ptr 
builtin_mul (const call_arguments& args)
{
    const auto args_size = args.size ();
    if (!(args_size > 0))
        raise<mal_exception_eval_invalid_arg> ();

    int retVal = 1;
    for (size_t i = 0; i < args_size; ++i)
        retVal *= args[i].as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();

    return std::make_unique<ast_node_atom_int> (retVal);
}

///////////////////////////////
/// environment class
///////////////////////////////
environment::environment (const environment* outer)
    : m_outer (outer)
{}

///////////////////////////////
const environment*
environment::find (const std::string &symbol) const
{
    if (m_data.count (symbol) > 0)
        return this;
    if (m_outer)
        return m_outer->find (symbol);

    return nullptr;
}

///////////////////////////////
void
environment::set (const std::string& symbol, ast_node::ptr val)
{
    m_data[symbol] = val;
}

///////////////////////////////
ast_node::ptr
environment::get (const std::string& symbol) const
{
    auto it = m_data.find (symbol);
    if (it != m_data.end ())
        return it->second;

    if (m_outer)
        return m_outer->get (symbol);

    return nullptr;
}

///////////////////////////////
ast_node::ptr
environment::get_or_throw (const std::string& symbol) const
{
    auto it = m_data.find (symbol);
    if (it != m_data.end ())
        return it->second;

    if (m_outer)
        return m_outer->get_or_throw (symbol);

    raise<mal_exception_eval_no_symbol> (symbol);
    return nullptr;
}
