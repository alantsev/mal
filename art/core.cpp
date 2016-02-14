#include "core.h"
#include "reader.h"

#include <string>
#include <fstream>
#include <streambuf>

namespace
{

///////////////////////////////
inline ast_node::ptr
ast_node_from_bool (bool f)
{
  return f ? ast_node::true_node : ast_node::false_node;
}

///////////////////////////////
inline int
arg_to_int (const call_arguments& args, size_t i)
{
  return args[i]->as_or_throw<ast_node_int, mal_exception_eval_not_int> ()->value ();
}

///////////////////////////////
template <typename CompareFn>
ast_node::ptr
compare_int_impl (const call_arguments& args, CompareFn&& fn)
{
  const auto args_size = args.size ();
  if (args_size != 2)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool ( fn (arg_to_int (args, 0), arg_to_int (args, 1)));
}

///////////////////////////////
ast_node::ptr
builtin_plus (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size < 1)
    raise<mal_exception_eval_invalid_arg> ();

  int retVal = 0;
  for (size_t i = 0; i < args_size; ++i)
    retVal += arg_to_int(args, i);

  return std::make_shared<ast_node_int> (retVal);
}

///////////////////////////////
ast_node::ptr
builtin_minus (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size < 1 || args_size > 2)
    raise<mal_exception_eval_invalid_arg> ();

  int retVal = (args_size == 1) ? -arg_to_int (args, 0) : arg_to_int (args, 0) - arg_to_int (args, 1);
  return std::make_shared<ast_node_int> (retVal);
}

///////////////////////////////
ast_node::ptr
builtin_div (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 2)
    raise<mal_exception_eval_invalid_arg> ();

  int retVal = arg_to_int(args, 0) / arg_to_int(args, 1);
  return std::make_shared<ast_node_int> (retVal);
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
    retVal *= arg_to_int(args, i);

  return std::make_shared<ast_node_int> (retVal);
}

///////////////////////////////
ast_node::ptr
builtin_list (const call_arguments& args)
{
  const auto args_size = args.size ();

  auto retVal = std::make_shared<ast_node_list> ();
  for (size_t i = 0; i < args_size; ++i)
    retVal->add_child (args[i]);

  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_is_list (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool (args[0]->type () == node_type_enum::LIST);
}

///////////////////////////////
ast_node::ptr
builtin_is_empty (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  auto arg_list = args[0]->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();
  return ast_node_from_bool (arg_list->size () == 0);
}

///////////////////////////////
ast_node::ptr
builtin_count (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  int count = 0;
  if (args[0]->type () != node_type_enum::NIL)
  {
    auto arg_list = args[0]->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();
    count = arg_list->size ();
  }

  return std::make_shared<ast_node_int> (count);

}

///////////////////////////////
ast_node::ptr
builtin_compare (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  2)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool (equals (*args[0], *args[1]));
}

///////////////////////////////
ast_node::ptr
builtin_less (const call_arguments& args)
{
  return compare_int_impl (args, [](int first, int second) { return first < second; });
}

///////////////////////////////
ast_node::ptr
builtin_less_or_eq (const call_arguments& args)
{
  return compare_int_impl (args, [](int first, int second) { return first <= second; });
}

///////////////////////////////
ast_node::ptr
builtin_greater (const call_arguments& args)
{
  return compare_int_impl (args, [](int first, int second) { return first > second; });
}

///////////////////////////////
ast_node::ptr
builtin_greater_or_eq (const call_arguments& args)
{
  return compare_int_impl (args, [](int first, int second) { return first >= second; });
}

///////////////////////////////
ast_node::ptr
builtin_pr_str (const call_arguments& args)
{
  const auto args_size = args.size ();

  std::string retVal;
  for (size_t i = 0; i < args_size; ++i)
  {
    if (i > 0)
      retVal += " ";
    retVal += args[i]->to_string (true);
  }

  return std::make_shared<ast_node_string> (std::move (retVal));
}

///////////////////////////////
ast_node::ptr
builtin_str (const call_arguments& args)
{
  const auto args_size = args.size ();

  std::string retVal;
  for (size_t i = 0; i < args_size; ++i)
  {
    retVal += args[i]->to_string (false);
  }

  return std::make_shared<ast_node_string> (std::move (retVal));
}

///////////////////////////////
ast_node::ptr
builtin_prn (const call_arguments& args)
{
  const auto args_size = args.size ();

  std::string printVal;
  for (size_t i = 0; i < args_size; ++i)
  {
    if (i > 0)
      printVal += " ";
    printVal += args[i]->to_string (true);
  }
  printf("%s\n", printVal.c_str ());

  return ast_node::nil_node;
}

///////////////////////////////
ast_node::ptr
builtin_println (const call_arguments& args)
{
  const auto args_size = args.size ();

  std::string printVal;
  for (size_t i = 0; i < args_size; ++i)
  {
    if (i > 0)
      printVal += " ";
    printVal += args[i]->to_string (false);
  }
  printf("%s\n", printVal.c_str ());

  return ast_node::nil_node;
}

///////////////////////////////
ast_node::ptr
builtin_read_string (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  auto strVal = args[0]->as_or_throw<ast_node_string, mal_exception_eval_not_string> ();
  return read_str (strVal->value ());
}

///////////////////////////////
ast_node::ptr
builtin_slurp (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  auto strVal = args[0]->as_or_throw<ast_node_string, mal_exception_eval_not_string> ();
  std::ifstream infile(strVal->value ());
  if (!infile)
    raise<mal_exception_eval_invalid_arg> ("file not found");

  std::string allText;

  infile.seekg(0, std::ios::end);   
  allText.reserve(infile.tellg());
  infile.seekg(0, std::ios::beg);

  allText.assign((std::istreambuf_iterator<char>(infile)),
                  std::istreambuf_iterator<char>());

  return std::make_shared<ast_node_string> (std::move (allText));
}

} // end of anonymous namespace

///////////////////////////////
core::core (environment::ptr root_env)
{
  env_add_builtin ("+", builtin_plus);
  env_add_builtin ("-", builtin_minus);
  env_add_builtin ("*", builtin_mul);
  env_add_builtin ("/", builtin_div);

  env_add_builtin ("list", builtin_list);
  env_add_builtin ("list?", builtin_is_list);
  env_add_builtin ("empty?", builtin_is_empty);
  env_add_builtin ("count", builtin_count);
  env_add_builtin ("=", builtin_compare);

  env_add_builtin ("<", builtin_less);
  env_add_builtin ("<=", builtin_less_or_eq);
  env_add_builtin (">", builtin_greater);
  env_add_builtin (">=", builtin_greater_or_eq);

  env_add_builtin ("pr-str", builtin_pr_str);
  env_add_builtin ("str", builtin_str);
  env_add_builtin ("prn", builtin_prn);
  env_add_builtin ("println", builtin_println);

  env_add_builtin ("read-string", builtin_read_string);
  env_add_builtin ("slurp", builtin_slurp);

  for (auto&& c : content ())
  {
    root_env->set (c.first, c.second);
  }
}

