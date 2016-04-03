#include "ast_details.h"
#include "environment.h"

///////////////////////////////
/// ast_node_list class
///////////////////////////////
std::string 
ast_node_list::to_string (bool print_readable) const // override
{
  std::string retVal = "(";
  for (size_t i = 0, e = m_children.size (); i < e; ++i)
  {
    auto &&p = m_children [i];
    if (i != 0)
      retVal += " ";
    retVal += p->to_string (print_readable);
  }
  retVal += ")";

  return retVal;
}

///////////////////////////////
/// ast_node_vector class
///////////////////////////////
std::string 
ast_node_vector::to_string (bool print_readable) const // override
{
  std::string retVal = "[";
  for (size_t i = 0, e = m_children.size (); i < e; ++i)
  {
    auto &&p = m_children [i];
    if (i != 0)
      retVal += " ";
    retVal += p->to_string (print_readable);
  }
  retVal += "]";

  return retVal;
}

///////////////////////////////
/// ast_node_symbol class
///////////////////////////////
ast_node_symbol::ast_node_symbol (std::string a_symbol)
  : m_symbol (std::move (a_symbol))
{}

///////////////////////////////
std::string
ast_node_symbol::to_string (bool print_readable) const // override
{
  // FIXME
  return m_symbol;
}

///////////////////////////////
/// ast_node_int class
///////////////////////////////
ast_node_int::ast_node_int (int64_t a_value)
  : m_value (a_value)
{}

///////////////////////////////
std::string
ast_node_int::to_string (bool print_readable) const // override
{
  // FIXME
  return std::to_string (m_value);
}

///////////////////////////////
/// ast_node_nil class
///////////////////////////////
std::string
ast_node_nil::to_string (bool print_readable) const
{
	return "nil";
}

///////////////////////////////
/// ast_node_callable_lambda class
///////////////////////////////
ast_node_callable_lambda::ast_node_callable_lambda (ast_node::ptr binds, ast_node::ptr ast, environment::const_ptr outer_env)
  : m_binds (binds)
  , m_ast (ast)
  , m_outer_env (outer_env)
{
  const auto bind_type = m_binds->type ();
  if (bind_type != node_type_enum::LIST && bind_type != node_type_enum::VECTOR)
    raise<mal_exception_eval_not_list> (m_binds->to_string ());

  m_binds_as_container = static_cast<const ast_node_container_base*> (m_binds.get ());
}

///////////////////////////////
tco
ast_node_callable_lambda::call_tco (const call_arguments& args) const
{
  auto env = environment::make (*m_binds_as_container, args, m_outer_env);
  return tco{m_ast, env, nullptr};
}
