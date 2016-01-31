#include "MAL.h"
#include "environment.h"
#include "ast_details.h"

#include <iostream>

///////////////////////////////
/// environment class
///////////////////////////////
environment::environment (hide_me, environment::const_ptr outer)
  : m_outer (outer)
{}

///////////////////////////////
environment::environment (hide_me, const ast_node_container_base& binds, const call_arguments& exprs, environment::const_ptr outer)
  : m_outer (outer)
{
  auto assign_variadic_param = [&] (size_t bind_index) 
  {
    assert (bind_index > 0);

    if (bind_index + 1 != binds.size ())
      raise<mal_exception_eval_invalid_arg> ();

    auto symbol = binds[bind_index]->template as_or_throw<ast_node_atom_symbol, mal_exception_eval_not_symbol> ()->symbol ();
    if (symbol == "&")
      raise<mal_exception_eval_invalid_arg> ();

    auto list = std::make_shared<ast_node_list> ();
    for (size_t i = bind_index - 1, e = exprs.size (); i < e; ++i)
    {
      list->add_child (exprs[i]);
    }

    m_data[symbol] = list;
  };

  //
  for (size_t i = 0, e = binds.size (); i < e; ++i)
  {
    auto symbol = binds[i]->template as_or_throw<ast_node_atom_symbol, mal_exception_eval_not_symbol> ()->symbol ();

    if (symbol == "&")
    {
      assign_variadic_param (i + 1);
      return;
    }
    m_data[symbol] = exprs[i];
  }

  if (binds.size () != exprs.size ())
    raise<mal_exception_eval_invalid_arg> ();
}

///////////////////////////////
environment::const_ptr
environment::find (const std::string &symbol) const
{
  if (m_data.count (symbol) > 0)
    return shared_from_this ();
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
