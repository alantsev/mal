#include "core.h"

namespace
{

///////////////////////////////
ast_node::ptr 
builtin_plus (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size < 1)
  raise<mal_exception_eval_invalid_arg> ();

  int retVal = 0;
  for (size_t i = 0; i < args_size; ++i)
  retVal += args[i]->as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();

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
  return args[i]->as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();
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
  return args[i]->as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();
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
  retVal *= args[i]->as_or_throw<ast_node_atom_int, mal_exception_eval_not_int> ()->value ();

  return std::make_unique<ast_node_atom_int> (retVal);
}

} // end of anonymous namespace

///////////////////////////////
core::core ()
{
  env_add_builtin ("+", builtin_plus);
  env_add_builtin ("-", builtin_minus);
  env_add_builtin ("*", builtin_mul);
  env_add_builtin ("/", builtin_div);
}

///////////////////////////////
void
core:: env_add_builtin (const std::string& symbol, ast_node_callable_builtin::builtin_fn fn)
{
  m_content[symbol] = std::make_unique<ast_node_callable_builtin> (symbol, fn);
}

