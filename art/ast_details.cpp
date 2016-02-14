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
/// ast_node_atom_symbol class
///////////////////////////////
ast_node_atom_symbol::ast_node_atom_symbol (std::string a_symbol)
  : m_symbol (std::move (a_symbol))
{}

///////////////////////////////
std::string
ast_node_atom_symbol::to_string (bool print_readable) const // override
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
ast_node_atom_int::to_string (bool print_readable) const // override
{
  // FIXME
  return std::to_string (m_value);
}

///////////////////////////////
/// ast_node_atom_nil class
///////////////////////////////
std::string
ast_node_atom_nil::to_string (bool print_readable) const
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
  std::shared_ptr<ast_node_list> child { new ast_node_list {} };
  ast_node_list* child_ref = child.get ();

  m_current_stack.back ()->add_child (child);
  m_current_stack.push_back (child_ref);
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::close_list ()
{
  m_current_stack.back ()->as_or_throw<ast_node_list, mal_exception_parse_error> ();
  m_current_stack.pop_back ();
  if (m_current_stack.size () == 0) 
    raise<mal_exception_parse_error> ();

  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::open_vector ()
{
  std::shared_ptr<ast_node_vector> child { new ast_node_vector {} };
  ast_node_vector* child_ref = child.get ();

  m_current_stack.back ()->add_child (child);
  m_current_stack.push_back (child_ref);
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::close_vector ()
{
  m_current_stack.back ()->as_or_throw<ast_node_vector, mal_exception_parse_error> ();
  m_current_stack.pop_back ();
  if (m_current_stack.size () == 0) 
    raise<mal_exception_parse_error> ();

  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::add_symbol (std::string value)
{
  ast_node::ptr child { new ast_node_atom_symbol {std::move (value)} };
  m_current_stack.back ()->add_child (child);
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::add_int (int value)
{
  ast_node::ptr child { new ast_node_atom_int { value } };
  m_current_stack.back ()->add_child (child);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::add_bool (bool value)
{
  m_current_stack.back ()->add_child (value ? ast_node::true_node : ast_node::false_node);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::add_nil ()
{
  m_current_stack.back ()->add_child (ast_node::nil_node);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::add_string (std::string str)
{
  ast_node::ptr child { new ast_node_atom_string {std::move (str)} };
  m_current_stack.back ()->add_child (child);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::start_string ()
{
  m_picewise_string = "";
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::append_string (const std::string &piece)
{
  m_picewise_string += piece;
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::finish_string ()
{
  add_string (m_picewise_string);
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

  auto root = m_current_stack.back()->as_or_throw<ast_node_list, mal_exception_parse_error> ();

  assert (root->size () > 0);
  ast_node::ptr retVal = (*root) [0];
  m_current_stack.clear ();
  m_meta_root.reset ();

  return retVal;
}
