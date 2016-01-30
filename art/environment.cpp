#include "MAL.h"
#include "environment.h"

#include <iostream>

///////////////////////////////
/// environment class
///////////////////////////////
environment::environment (hide_me, environment::const_ptr outer)
  : m_outer (outer)
{}

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
