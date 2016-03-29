#include "MAL.h"
#include "exceptions.h"
#include "ast.h"
#include "ast_details.h"
#include "reader.h"
#include "environment.h"
#include "core.h"

#include <readline/readline.h>
#include <readline/history.h>

#include <cstdio>
#include <iostream>
#include <string>

///////////////////////////////
std::string 
readline (const std::string& prompt)
{
  std::string retVal;
  do
  {
    char * line = readline(prompt.c_str ());
    if (!line)
      raise<mal_exception_stop> ();

    if (*line) 
      add_history(line);

    retVal = line;
    free(line);
  } while (retVal.empty ());
  return retVal;
}

///////////////////////////////
void
printline (const std::string& line)
{
  std::cout << line << std::endl;
};

///////////////////////////////
ast
EVAL (ast tree, environment::ptr a_env);

///////////////////////////////
ast
READ (const std::string& prompt)
{
  return read_str ( readline (prompt));
}

///////////////////////////////
ast
call_fn (const ast_node_list* callable_list)
{
  const size_t list_size = callable_list->size ();
  if (list_size == 0)
    raise<mal_exception_eval_not_callable> (callable_list->to_string ());

  auto && callable_node = (*callable_list)[0]->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();

  auto call_arg = callable_node->call_tco (call_arguments (callable_list, 1, list_size - 1));
  auto retVal = std::get<2> (call_arg);

  if (retVal)
    return retVal;

  return EVAL (std::get<0> (call_arg), std::get<1> (call_arg));
}

///////////////////////////////
ast
eval_ast (ast tree, environment::ptr a_env); // fwd decl

///////////////////////////////
// FIXME - make it loop over 
//   - eval_ast
//   - apply
// 
ast
EVAL (ast tree, environment::ptr a_env)
{
  if (tree->type () != node_type_enum::LIST)
    return eval_ast (tree, a_env);

  // default apply - call fn, first argument is callable
  auto fn_default_list_apply = [&] ()
  {
    ast_node::ptr new_node = eval_ast (tree, a_env);
    auto new_node_list = new_node->as_or_throw<ast_node_list, mal_exception_eval_not_list> ();
    return call_fn (new_node_list);
  };

  // not as_or_throw - we know the type
  auto root_list = tree->as<ast_node_list> ();
  if (root_list->empty ())
    return tree;

  //
  auto fn_handle_def = [root_list, &a_env]()
  {
    if (root_list->size () != 3)
      raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

    const auto& key = (*root_list)[1]->as_or_throw<ast_node_symbol, mal_exception_eval_invalid_arg> ()->symbol ();

    ast_node::ptr value = EVAL ((*root_list)[2], a_env);
    a_env->set (key, value);
    return value;
  };

  // 
  auto fn_handle_let = [root_list, &a_env]()
  {
    if (root_list->size () != 3)
      raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

    const ast_node_container_base* let_bindings = nullptr;
    const auto root_list_arg_1 = (*root_list)[1];
    switch (root_list_arg_1->type ())
    {
    case node_type_enum::LIST:
    case node_type_enum::VECTOR:
      let_bindings = root_list_arg_1->as<ast_node_container_base> ();
      break;
    default:
      raise<mal_exception_eval_invalid_arg> (root_list_arg_1->to_string ());
    };

    //
    auto let_env = environment::make (a_env);

    if (let_bindings->size () % 2 != 0)
      raise<mal_exception_eval_invalid_arg> (let_bindings->to_string ());
    
    for (size_t i = 0, e = let_bindings->size(); i < e; i += 2)
    {
      const auto& key = (*let_bindings)[i]->as_or_throw<ast_node_symbol, mal_exception_eval_invalid_arg> ()->symbol ();
      ast_node::ptr value = EVAL ((*let_bindings)[i + 1], let_env);

      let_env->set (key, value);
    }

    return EVAL ((*root_list)[2], let_env);
  };

  auto fn_handle_do = [root_list, &a_env]()
  {
    const size_t list_size = root_list->size ();
    if (list_size < 2)
      raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

    for (size_t i = 1, e = list_size - 1; i < e; ++i)
    {
      /*retVal = */EVAL ((*root_list)[i], a_env);
    }

    return EVAL ((*root_list)[list_size - 1], a_env);
  };

  auto fn_handle_if = [root_list, &a_env] () -> ast
  {
    const size_t list_size = root_list->size ();
    if (list_size < 3 || list_size > 4)
      raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

    ast_node::ptr condNode = EVAL ((*root_list)[1], a_env);
    const bool cond = !(condNode == ast_node::nil_node) && !(condNode == ast_node::false_node);

    if (cond)
      return EVAL((*root_list)[2], a_env);
    else if (list_size == 4)
      return EVAL((*root_list)[3], a_env);

    return ast_node::nil_node;
  };

  auto fn_handle_fn = [root_list, &a_env] () -> ast
  {
    const size_t list_size = root_list->size ();
    if (list_size != 3)
      raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

    auto&& bindsNode = (*root_list)[1];
    auto&& astNode = (*root_list)[2];

    ast_node::ptr retVal = std::make_shared<ast_node_callable_lambda> (bindsNode, astNode, a_env);
    return retVal;
  };

  auto first = (*root_list)[0];
  if (first->type () == node_type_enum::SYMBOL)
  {
    // apply special symbols
    // not as_or_throw - we know the type
    const auto first_symbol = first->as<ast_node_symbol> ();
    const auto& symbol = first_symbol->symbol ();

    if (symbol == "def!")
    {
      return fn_handle_def ();
    }
    else if (symbol == "let*")
    {
      return fn_handle_let ();
    }
    else if (symbol == "do")
    {
      return fn_handle_do ();
    }
    else if (symbol == "if")
    {
      return fn_handle_if ();
    }
    else if (symbol == "fn*")
    {
      return fn_handle_fn ();
    }
  }

  return fn_default_list_apply ();
}


///////////////////////////////
ast
eval_ast (ast tree, environment::ptr a_env)
{
  switch (tree->type ())
  {
  case node_type_enum::SYMBOL:
    {
      // not as_or_throw - we know the type
      const auto& node_symbol = tree->as<ast_node_symbol> ();
      return a_env->get_or_throw (node_symbol->symbol ());

    }
  case node_type_enum::LIST:
  case node_type_enum::VECTOR:
    {
      // not as_or_throw - we know the type
      const auto& node_container = tree->as<ast_node_container_base> ();
      // TODO - add here optimization to do not clone underlying node if the current pointer is unique!
      return node_container->map (
        [&a_env] (ast_node::ptr v) { 
          return EVAL (v, a_env);
        });
    }
  case node_type_enum::HT_LIST:
    {
      // not as_or_throw - we know the type
      const auto& node_container = tree->as<ast_node_ht_list> ();
      // TODO - add here optimization to do not clone underlying node if the current pointer is unique!
      auto evaledList = node_container->map (
        [&a_env] (ast_node::ptr v) { 
          return EVAL (v, a_env);
        });
      ast_node::ptr retVal = mal::make_hashmap (evaledList->as<ast_node_ht_list> ());
      return retVal;
    }

  default:
    break;
  }

  return tree;
}

///////////////////////////////
void
PRINT (ast tree)
{
  printline (pr_str (tree, true));
}

///////////////////////////////
void
rep (const std::string& prompt, environment::ptr env)
{
  PRINT (EVAL ( READ (prompt), env));
}

///////////////////////////////
int
main(int, char**)
{
  try
  {
    const std::string prompt = "user> ";

    auto env = environment::make ();
    core ns (env);

    // define not function
    EVAL (read_str ("(def! not (fn* (a) (if a false true)))"), env);

    // repl
    for (;;)
    {
      try
      {

        rep (prompt, env);
      }
      catch (const mal_exception_parse_error& ex)
      {
        printline (std::string ("parse error: ") + ex.what ());
      }
      catch (const mal_exception_eval_not_callable& ex)
      {
        printline (std::string ("not callable: ") + ex.what ());
      }
      catch (const mal_exception_eval_not_int& ex)
      {
        printline (std::string ("not int: ") + ex.what ());
      }
      catch (const mal_exception_eval_not_list& ex)
      {
        printline (std::string ("not list: ") + ex.what ());
      }
      catch (const mal_exception_eval_not_symbol& ex)
      {
        printline (std::string ("not symbol: ") + ex.what ());
      }
      catch (const mal_exception_eval_invalid_arg& ex)
      {
        printline (std::string ("invalid arguments: ") + ex.what ());
      }
      catch (const mal_exception_eval_no_symbol& ex)
      {
        printline (std::string ("no symbol: ") + ex.what ());
      }
    }
  }
  catch (const mal_exception_stop&)
  {
  }
  std::cout << std::endl;
}
