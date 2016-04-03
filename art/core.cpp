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

  auto retVal = mal::make_list ();
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

///////////////////////////////
ast_node::ptr
builtin_atom (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return std::make_shared<ast_node_atom> (args[0]);
}

///////////////////////////////
ast_node::ptr
builtin_is_atom (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool (args[0]->type () == node_type_enum::ATOM);
}

///////////////////////////////
ast_node::ptr
builtin_deref (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return args[0]->as_or_throw<ast_node_atom, mal_exception_eval_not_atom> ()->get_value ();
}

///////////////////////////////
ast_node::ptr
builtin_reset (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  2)
    raise<mal_exception_eval_invalid_arg> ();

  auto atom = args[0]->as_or_throw<ast_node_atom, mal_exception_eval_not_atom> ();
  auto val = args[1];

  atom->set_value (val);
  return val;
}

///////////////////////////////
ast_node::ptr
builtin_cons (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  2)
    raise<mal_exception_eval_invalid_arg> ();

  auto retVal = mal::make_list ();
  retVal->add_child (args [0]);
  auto l = args[1]->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();
  for (size_t j = 0, je = l->size (); j < je; ++j)
  {
    retVal->add_child ((*l) [j]);
  }
  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_concat (const call_arguments& args)
{
  const auto args_size = args.size ();

  auto retVal = mal::make_list ();
  for (size_t i = 0; i < args_size; ++i)
  {
    auto l = args[i]->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();
    for (size_t j = 0, je = l->size (); j < je; ++j)
    {
      retVal->add_child ((*l) [j]);
    }
  }
  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_nth (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  2)
    raise<mal_exception_eval_invalid_arg> ();

  auto l = args[0]->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();
  auto n = arg_to_int (args, 1);

  if (n < 0 || n >= l->size ())
    raise<mal_exception_eval_invalid_arg> ("index out of bounds");

  return (*l) [n];
}

///////////////////////////////
ast_node::ptr
builtin_first (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 1)
    raise<mal_exception_eval_invalid_arg> ();

  auto first = args[0];
  if (first == ast_node::nil_node)
    return ast_node::nil_node;

  auto l = first->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();

  if (l->empty ())
    return ast_node::nil_node;

  return (*l) [0];
}

///////////////////////////////
ast_node::ptr
builtin_rest (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 1)
    raise<mal_exception_eval_invalid_arg> ();

  auto retVal = mal::make_list ();
  auto first = args[0];
  if (first != ast_node::nil_node)
  {
    auto l = first->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();
    const auto lSize = l->size ();

    for (size_t i = 1; i < lSize; ++i)
    {
      retVal->add_child ((*l) [i]);
    }
  }

  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_throw (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  throw args[0];
  return ast_node::nil_node;
}

///////////////////////////////
ast_node::ptr
builtin_is_symbol (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool (args[0]->type () == node_type_enum::SYMBOL);
}

///////////////////////////////
ast_node::ptr
builtin_symbol (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return mal::make_symbol (args[0]->as_or_throw<ast_node_string, mal_exception_eval_not_string> ()->value ());
}

///////////////////////////////
ast_node::ptr
builtin_keyword (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return mal::make_keyword (":" + args[0]->as_or_throw<ast_node_string, mal_exception_eval_not_string> ()->value ());
}

///////////////////////////////
ast_node::ptr
builtin_is_keyword (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool (args[0]->type () == node_type_enum::KEYWORD);
}

///////////////////////////////
ast_node::ptr
builtin_vector (const call_arguments& args)
{
  const auto args_size = args.size ();

  auto retVal = mal::make_vector ();
  for (size_t i = 0; i < args_size; ++i)
    retVal->add_child (args[i]);

  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_is_vector (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool (args[0]->type () == node_type_enum::VECTOR);
}

///////////////////////////////
ast_node::ptr
builtin_hashmap (const call_arguments& args)
{
  return mal::make_hashmap (args);
}

///////////////////////////////
ast_node::ptr
builtin_is_hashmap (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size !=  1)
    raise<mal_exception_eval_invalid_arg> ();

  return ast_node_from_bool (args[0]->type () == node_type_enum::HASHMAP);
}

///////////////////////////////
ast_node::ptr
builtin_assoc (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size <  1)
    raise<mal_exception_eval_invalid_arg> ();

  auto retVal = args[0]->as_or_throw<ast_node_hashmap, mal_exception_eval_not_hashmap> ()->clone ();

  if (args_size % 2 == 0)
    raise<mal_exception_parse_error> ("odd number of elements in hashmap");

  for (size_t i = 1; i < args_size; i += 2)
  {
    retVal->as<ast_node_hashmap> ()->insert (args[i], args[i + 1]);
  }

  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_dissoc (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size < 1)
    raise<mal_exception_eval_invalid_arg> ();

  auto retVal = args[0]->as_or_throw<ast_node_hashmap, mal_exception_eval_not_hashmap> ()->clone ();

  for (size_t i = 1; i < args_size; ++i)
  {
    retVal->as<ast_node_hashmap> ()->erase (args[i]);
  }

  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_get (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 2)
    raise<mal_exception_eval_invalid_arg> ();

  auto hashmap = args[0];
  if (hashmap->type () == node_type_enum::NIL)
    return ast_node::nil_node;

  return hashmap->as_or_throw<ast_node_hashmap, mal_exception_eval_not_hashmap> ()->get (args[1]);
}

///////////////////////////////
ast_node::ptr
builtin_is_contains (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 2)
    raise<mal_exception_eval_invalid_arg> ();

  auto hashmap = args[0];
  if (hashmap->type () == node_type_enum::NIL)
    return ast_node::false_node;

  auto hashmap_casted = hashmap->as_or_throw<ast_node_hashmap, mal_exception_eval_not_hashmap> ();

  return ast_node_from_bool (hashmap_casted->has (args[1]));
}

///////////////////////////////
ast_node::ptr
builtin_keys (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 1)
    raise<mal_exception_eval_invalid_arg> ();

  auto hashmap = args[0]->as_or_throw<ast_node_hashmap, mal_exception_eval_not_hashmap> ();
  auto retVal = mal::make_list ();

  hashmap->for_each ([&] (ast_node::ptr k, ast_node::ptr) { retVal->add_child (k); });
  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_vals (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 1)
    raise<mal_exception_eval_invalid_arg> ();

  auto hashmap = args[0]->as_or_throw<ast_node_hashmap, mal_exception_eval_not_hashmap> ();
  auto retVal = mal::make_list ();

  hashmap->for_each ([&] (ast_node::ptr, ast_node::ptr v) { retVal->add_child (v); });
  return retVal;
}

///////////////////////////////
ast_node::ptr
builtin_is_sequential (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 1)
    raise<mal_exception_eval_invalid_arg> ();

  auto nodeType = args[0]->type ();
  const bool isSeq = nodeType == node_type_enum::LIST || nodeType == node_type_enum::VECTOR;
  return ast_node_from_bool (isSeq);
}

///////////////////////////////
ast_node::ptr
builtin_meta (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 1)
    raise<mal_exception_eval_invalid_arg> ();

  return args[0]->meta ();
}

///////////////////////////////
ast_node::ptr
builtin_with_meta (const call_arguments& args)
{
  const auto args_size = args.size ();
  if (args_size != 2)
    raise<mal_exception_eval_invalid_arg> ();

  auto meta = args[1];
  if (meta->type () != node_type_enum::HASHMAP)
    raise<mal_exception_eval_invalid_arg> (meta->to_string ());

  return args[0]->clone_with_meta (meta);
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

  env_add_builtin ("atom", builtin_atom);
  env_add_builtin ("atom?", builtin_is_atom);
  env_add_builtin ("deref", builtin_deref);
  env_add_builtin ("reset!", builtin_reset);

  env_add_builtin ("cons", builtin_cons);
  env_add_builtin ("concat", builtin_concat);

  env_add_builtin ("nth", builtin_nth);
  env_add_builtin ("first", builtin_first);
  env_add_builtin ("rest", builtin_rest);

  env_add_builtin ("throw", builtin_throw);
  env_add_builtin ("symbol?", builtin_is_symbol);
  env_add_builtin ("symbol", builtin_symbol);
  env_add_builtin ("keyword", builtin_keyword);
  env_add_builtin ("keyword?", builtin_is_keyword);
  env_add_builtin ("vector", builtin_vector);
  env_add_builtin ("vector?", builtin_is_vector);
  env_add_builtin ("hash-map", builtin_hashmap);
  env_add_builtin ("map?", builtin_is_hashmap);
  env_add_builtin ("assoc", builtin_assoc);
  env_add_builtin ("dissoc", builtin_dissoc);
  env_add_builtin ("get", builtin_get);
  env_add_builtin ("contains?", builtin_is_contains);
  env_add_builtin ("keys", builtin_keys);
  env_add_builtin ("vals", builtin_vals);
  env_add_builtin ("vals", builtin_vals);
  env_add_builtin ("sequential?", builtin_is_sequential);

  env_add_builtin ("meta", builtin_meta);
  env_add_builtin ("with-meta", builtin_with_meta);

  for (auto&& c : content ())
  {
    root_env->set (c.first, c.second);
  }
}

