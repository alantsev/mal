#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>
#include <string>

#include "MAL.h"

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
ast_node::ptr
apply (const ast_node_list* callable_list)
{
  const size_t list_size = callable_list->size ();
  if (list_size == 0)
    raise<mal_exception_eval_not_callable> (callable_list->to_string ());

  auto && callable_node = (*callable_list)[0]->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();

  return callable_node->call (ast_node_list::list_call_arguments (callable_list, 1, list_size - 1));
}

///////////////////////////////
ast_node::ptr
eval_ast (ast_node::ptr node, environment& a_env); // fwd decl

///////////////////////////////
ast_node::ptr
eval_impl (ast_node::ptr root, environment& a_env)
{
  if (root->type () != node_type_enum::LIST)
    return eval_ast (std::move (root), a_env);

  // not as_or_throw - we know the type
  auto root_list = root->as<ast_node_list> ();

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
        // TODO - do we really need clone here???
        ast_node::ptr value = eval_impl ((*root_list)[2]->clone (), a_env);

        // TODO - get rid of clone here!!!
        return a_env.set (key, std::move (value))->clone ();
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
        environment let_env (std::addressof (a_env));

        if (let_bindings->size () % 2 != 0)
          raise<mal_exception_eval_invalid_arg> (let_bindings->to_string ());
        
        for (size_t i = 0, e = let_bindings->size(); i < e; i += 2)
        {
          const auto& key = (*let_bindings)[i]->as_or_throw<ast_node_atom_symbol, mal_exception_eval_invalid_arg> ()->symbol ();
          // TODO - do we really need clone here???
          ast_node::ptr value = eval_impl ((*let_bindings)[i + 1]->clone (), let_env);

          let_env.set (key, std::move (value));
        }

        // TODO - get rid of clone here
        return eval_impl ((*root_list)[2]->clone (), let_env);
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
  ast_node::ptr new_node = eval_ast (std::move (root), a_env);
  auto new_node_list = new_node->as_or_throw<ast_node_list, mal_exception_eval_not_list> ();
  return apply (new_node_list);
}


///////////////////////////////
ast_node::ptr
eval_ast (ast_node::ptr node, environment& a_env)
{
  auto fn_handle_container = [&a_env](ast_node_container_base* container)
  {
    // FIXME - make here clone if necessary - before call
    container->map ([&a_env] (ast_node::ptr v) { return std::move (eval_impl (std::move (v), a_env));});
  };

  switch (node->type ())
  {
  case node_type_enum::SYMBOL:
    {
      // not as_or_throw - we know the type
      const auto& node_symbol = node->as<ast_node_atom_symbol> ();
      return a_env.get_or_throw (node_symbol->symbol ())->clone ();

    }
  case node_type_enum::LIST:
    {
      // not as_or_throw - we know the type
      fn_handle_container (node->as<ast_node_list> ());
      break;
    }
  case node_type_enum::VECTOR:
    {
      // not as_or_throw - we know the type
      fn_handle_container (node->as<ast_node_vector> ());
      break;
    }

  default:
    break;
  }

  return std::move (node);
}

///////////////////////////////
ast
EVAL (ast a_ast, environment& a_env)
{
  return { eval_impl (std::move (a_ast.m_root), a_env) };
}

///////////////////////////////
void
PRINT (ast&& a_ast)
{
  printline (pr_str (std::move (a_ast)));
}

///////////////////////////////
void
rep (const std::string& prompt, environment& env)
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

    environment env;
    env_add_builtin (env, "+", builtin_plus);
    env_add_builtin (env, "-", builtin_minus);
    env_add_builtin (env, "*", builtin_mul);
    env_add_builtin (env, "/", builtin_div);

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
