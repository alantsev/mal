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
#include <tuple>

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
READ (const std::string& prompt)
{
  return read_str ( readline (prompt));
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
  ///////////////////////////////
  // tree, env, retVal
  using tco = std::tuple <ast, environment::ptr, ast>;

  for (;;)
  {
    if (tree->type () != node_type_enum::LIST)
      return eval_ast (tree, a_env);

    // not as_or_throw - we know the type
    auto root_list = tree->as<ast_node_list> ();
    if (root_list->empty ())
      return tree;

    //
    auto fn_handle_def = [root_list, &a_env]()
    {
      if (root_list->size () != 3)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      const auto& key = (*root_list)[1]->as_or_throw<ast_node_atom_symbol, mal_exception_eval_invalid_arg> ()->symbol ();

      ast_node::ptr value = EVAL ((*root_list)[2], a_env);
      a_env->set (key, value);
      return value;
    };

    // tco
    auto fn_handle_let_tco = [root_list, &a_env]() -> tco
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
        const auto& key = (*let_bindings)[i]->as_or_throw<ast_node_atom_symbol, mal_exception_eval_invalid_arg> ()->symbol ();
        ast_node::ptr value = EVAL ((*let_bindings)[i + 1], let_env);

        let_env->set (key, value);
      }

      return {(*root_list)[2], let_env};
    };

    // tco
    auto fn_handle_apply_tco= [&tree, &a_env]() -> tco
    {
      ast_node::ptr new_node = eval_ast (tree, a_env);
      auto callable_list = new_node->as_or_throw<ast_node_list, mal_exception_eval_not_list> ();

      const size_t list_size = callable_list->size ();
      if (list_size == 0)
        raise<mal_exception_eval_not_callable> (callable_list->to_string ());

      auto && callable_node = (*callable_list)[0]->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();

      // FIXME - 
      return tco {nullptr, nullptr, callable_node->call (call_arguments (callable_list, 1, list_size - 1))};
    };

    // tco
    auto fn_handle_do_tco = [root_list, &a_env]() -> tco
    {
      const size_t list_size = root_list->size ();
      if (list_size < 2)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      for (size_t i = 1, e = list_size - 1; i < e; ++i)
      {
        /*retVal = */EVAL ((*root_list)[i], a_env);
      }

      return {(*root_list)[list_size - 1], a_env};
    };

    // tco
    auto fn_handle_if_tco = [root_list, &a_env] () -> tco
    {
      const size_t list_size = root_list->size ();
      if (list_size < 3 || list_size > 4)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      ast_node::ptr condNode = EVAL ((*root_list)[1], a_env);
      const bool cond = !(condNode == ast_node::nil_node) && !(condNode == ast_node::false_node);

      if (cond)
        return {(*root_list)[2], a_env};
      else if (list_size == 4)
        return {(*root_list)[3], a_env};

      return tco {nullptr, nullptr, ast_node::nil_node};
    };

    auto fn_handle_fn = [root_list, &a_env] () -> ast
    {
      const size_t list_size = root_list->size ();
      if (list_size != 3)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      auto&& bindsNode = (*root_list)[1];
      auto&& astNode = (*root_list)[2];

      auto evalFn = [] (ast_node::ptr tree, environment::ptr env) -> ast_node::ptr
      {
        return EVAL (tree, env);
      };

      ast_node::ptr retVal = std::make_shared<ast_node_callable_lambda> (bindsNode, astNode, a_env, evalFn);
      return retVal;
    };

    auto first = (*root_list)[0];
    if (first->type () == node_type_enum::SYMBOL)
    {
      // apply special symbols
      // not as_or_throw - we know the type
      const auto first_symbol = first->as<ast_node_atom_symbol> ();
      const auto& symbol = first_symbol->symbol ();

      if (symbol == "def!")
      {
        return fn_handle_def ();
      }
      else if (symbol == "let*")
      {
        std::tie (tree, a_env, std::ignore) = fn_handle_let_tco ();
        continue;
      }
      else if (symbol == "do")
      {
        std::tie (tree, a_env, std::ignore) = fn_handle_do_tco ();
        continue;
      }
      else if (symbol == "if")
      {
        ast retVal;
        std::tie (tree, a_env, retVal) = fn_handle_if_tco ();
        if (retVal)
          return retVal;
        continue;
      }
      else if (symbol == "fn*")
      {
        return fn_handle_fn ();
      }
    }

    // apply
    {
      ast retVal;
      std::tie (tree, a_env, retVal) = fn_handle_apply_tco ();
      if (retVal)
        return retVal;
      continue;
    }

  }
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
      const auto& node_symbol = tree->as<ast_node_atom_symbol> ();
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
    core ns;

    for (auto&& c : ns.content ())
    {
      env->set (c.first, c.second);
    }
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
