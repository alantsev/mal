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
READ (const std::string& prompt)
{
  return read_str ( readline (prompt));
}

///////////////////////////////
ast
apply (const ast_node_list* callable_list)
{
  const size_t list_size = callable_list->size ();
  if (list_size == 0)
    raise<mal_exception_eval_not_callable> (callable_list->to_string ());

  auto && callable_node = (*callable_list)[0]->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();

  return callable_node->call (call_arguments (callable_list, 1, list_size - 1));
}

///////////////////////////////
ast
eval_ast (ast tree, environment::ptr a_env); // fwd decl

///////////////////////////////
ast
EVAL (ast tree, environment::ptr a_env)
{
  if (tree->type () != node_type_enum::LIST)
    return eval_ast (tree, a_env);

  // not as_or_throw - we know the type
  auto root_list = tree->as<ast_node_list> ();

  // apply special symbols
  if (root_list->size () > 0)
  {
    auto first = (*root_list)[0];
    if (first->type () == node_type_enum::SYMBOL)
    {
      // not as_or_throw - we know the type
      const auto first_symbol = first->as<ast_node_atom_symbol> ();
      const auto& symbol = first_symbol->symbol ();

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
          let_bindings = root_list_arg_1->as<ast_node_list> ();
          break;
        case node_type_enum::VECTOR:
          let_bindings = root_list_arg_1->as<ast_node_vector> ();
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

        return EVAL ((*root_list)[2], let_env);
      };


      if (symbol == "def!")
      {
        return fn_handle_def ();
      }
      else if (symbol == "let*")
      {
        return fn_handle_let ();
      }
    }
  }

  // default apply
  ast_node::ptr new_node = eval_ast (tree, a_env);
  auto new_node_list = new_node->as_or_throw<ast_node_list, mal_exception_eval_not_list> ();
  return apply (new_node_list);
}


///////////////////////////////
ast
eval_ast (ast tree, environment::ptr a_env)
{
  auto fn_handle_container = [&a_env](const ast_node_container_base* container)
  {
    // TODO - add here optimization to do not clone underlying node if the current pointer is unique!
    return container->map ([&a_env] (ast_node::ptr v) { return EVAL (v, a_env);});
  };

  switch (tree->type ())
  {
  case node_type_enum::SYMBOL:
    {
      // not as_or_throw - we know the type
      const auto& node_symbol = tree->as<ast_node_atom_symbol> ();
      return a_env->get_or_throw (node_symbol->symbol ());

    }
  case node_type_enum::LIST:
    {
      // not as_or_throw - we know the type
      return fn_handle_container (tree->as<ast_node_list> ());
    }
  case node_type_enum::VECTOR:
    {
      // not as_or_throw - we know the type
      return fn_handle_container (tree->as<ast_node_vector> ());
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
  printline (pr_str (tree));
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
