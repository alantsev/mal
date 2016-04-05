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
  if (!line.empty ())
    std::cout << line << std::endl;
};

///////////////////////////////
ast
READ (const std::string& line)
{
  return read_str (line);
}

///////////////////////////////
ast
eval_ast (ast tree, environment::ptr a_env); // fwd decl

///////////////////////////////
ast
EVAL (ast tree, environment::ptr a_env); // fwd decl

///////////////////////////////
// tco
tco 
handle_quasiquote_impl (ast_node::ptr node, environment::ptr a_env)
{
  auto is_pair = [] (const ast_node::ptr& p) -> bool
  {
    auto p_list = p->as_or_zero<ast_node_container_base> ();
    return p_list ? p_list->size () != 0 : false;
  };

  if (!is_pair (node))
    return tco {nullptr, nullptr, node};

  // it's non-empy list
  auto nodeList = node->as<ast_node_container_base> ();
  const auto nodeListSize = nodeList->size ();
  auto nodeListFirstSym = (*nodeList)[0]->as_or_zero<ast_node_symbol> ();

  if (nodeListFirstSym && nodeListFirstSym->symbol () == "unquote")
  {
    if (nodeListSize != 2)
      raise<mal_exception_eval_invalid_arg> (nodeList->to_string ());
    return tco {(*nodeList)[1], a_env};
  }

  //
  auto mapFn = [&a_env] (ast_node::ptr v) -> ast_node::ptr { 
    ast_node::ptr retVal;
    ast_node::ptr newNode;
    environment::ptr newEnv;
    std::tie (newNode, newEnv, retVal) = handle_quasiquote_impl (v, a_env);
    if (retVal)
      return retVal;

    return EVAL (newNode, newEnv);
  };

  auto retVal = mal::make_list ();
  for (size_t i = 0; i < nodeListSize; ++i)
  {
    auto && v = (*nodeList) [i];

    // splice-unquote
    if (is_pair (v))
    {
      auto childNodeList = v->as<ast_node_container_base> ();
      auto childSymCom = (*childNodeList)[0]->as_or_zero<ast_node_symbol> ();
      if (childSymCom && childSymCom->symbol () == "splice-unquote")
      {
        const auto spliceUnquteSize = childNodeList->size ();
        if (spliceUnquteSize != 2)
          raise<mal_exception_eval_invalid_arg> (childNodeList->to_string ());

        auto evaledList = EVAL ((*childNodeList)[1], a_env);
        auto splicedList = evaledList->as_or_throw<ast_node_container_base, mal_exception_eval_not_list> ();

        for (size_t c = 0, ce = splicedList->size (); c < ce; ++c)
        {
          auto && cv = (*splicedList) [c];
          retVal->add_child (cv);
        }

        continue;
      }
    }

    // or add 
    retVal->add_child (mapFn (v));
  }

  return tco {nullptr, nullptr, retVal};
};

///////////////////////////////
tco 
handle_quasiquote (const ast_node_list *root_list, environment::ptr a_env)
{
  const size_t list_size = root_list->size ();
  if (list_size != 2)
    raise<mal_exception_eval_invalid_arg> (root_list->to_string ());
  return handle_quasiquote_impl ((*root_list)[1], a_env);
};

///////////////////////////////
ast_node::ptr
is_macro_call (ast tree, environment::ptr a_env)
{
  if (tree->type () != node_type_enum::LIST)
    return {};

  // not as_or_throw - we know the type
  auto root_list = tree->as<ast_node_list> ();
  if (root_list->empty ())
    return {};

  auto first = (*root_list)[0];
  if (first->type () != node_type_enum::SYMBOL)
    return {};

  const auto first_symbol = first->as<ast_node_symbol> ();
  auto macro_node = a_env->get (first_symbol->symbol ());

  if (!macro_node)
    return {};

  if (!macro_node->as_or_zero<ast_node_macro_call> ())
    return {};

  return macro_node;
}

///////////////////////////////
ast
macroexpand (ast tree, environment::ptr a_env)
{
  ast_node::ptr macro_call;
  while ( (macro_call = is_macro_call (tree, a_env)) != nullptr ) 
  {
    auto callable_list = tree->as<ast_node_list> ();
    const size_t list_size = callable_list->size ();
    assert (list_size > 0);

    ast treeApply;
    environment::ptr envApply;
    ast retVal;
    std::tie (treeApply, envApply, retVal) = macro_call->as<ast_node_macro_call> ()->callable_node ()->as<ast_node_callable> ()->call_tco (call_arguments (callable_list, 1, list_size - 1));

    assert (!retVal);
    tree = EVAL (treeApply, envApply);
  }
  return tree;
}

///////////////////////////////
ast
EVAL (ast tree, environment::ptr a_env)
{
  for (;;)
  {
    if (!tree)
      return tree;

    if (tree->type () != node_type_enum::LIST)
      return eval_ast (tree, a_env);

    tree = macroexpand (tree, a_env);

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

      const auto& key = (*root_list)[1]->as_or_throw<ast_node_symbol, mal_exception_eval_invalid_arg> ()->symbol ();

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
        const auto& key = (*let_bindings)[i]->as_or_throw<ast_node_symbol, mal_exception_eval_invalid_arg> ()->symbol ();
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

      return callable_node->call_tco (call_arguments (callable_list, 1, list_size - 1));
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

      ast_node::ptr retVal = std::make_shared<ast_node_callable_lambda> (bindsNode, astNode, a_env);
      return retVal;
    };

    auto fn_handle_quote = [root_list, &a_env] () -> ast
    {
      const size_t list_size = root_list->size ();
      if (list_size != 2)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      return (*root_list)[1];
    };

    //
    auto fn_handle_defmacro = [root_list, &a_env]()
    {
      if (root_list->size () != 3)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      const auto& key = (*root_list)[1]->as_or_throw<ast_node_symbol, mal_exception_eval_invalid_arg> ()->symbol ();

      ast_node::ptr value = EVAL ((*root_list)[2], a_env);
      auto macro_call = value->as_or_zero<ast_node_macro_call> ();
      if (macro_call)
        a_env->set (key, value);
      else
        a_env->set (key, std::make_shared<ast_node_macro_call> (value));
      return value;
    };

    //
    auto fn_handle_macroexpand = [root_list, &a_env] () -> ast
    {
      const size_t list_size = root_list->size ();
      if (list_size != 2)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      return macroexpand ((*root_list)[1], a_env);
    };

    // try
    auto fn_handle_try = [root_list, &a_env] () -> ast
    {
      const size_t list_size = root_list->size ();
      if (list_size != 3)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      auto eval_part = (*root_list)[1];
      auto catch_list = (*root_list)[2]->as_or_throw<ast_node_list, mal_exception_eval_invalid_arg> ();

      const size_t catch_list_size = catch_list->size ();
      if (catch_list_size != 3)
        raise<mal_exception_eval_invalid_arg> (root_list->to_string ());

      if ((*catch_list)[0]->as_or_throw<ast_node_symbol, mal_exception_eval_invalid_arg> ()->symbol () != "catch*")
        raise<mal_exception_eval_invalid_arg> (catch_list->to_string ());

      auto bind_ex_symbol = (*catch_list)[1]->as_or_throw<ast_node_symbol, mal_exception_eval_invalid_arg> ();
      auto catch_eval_part = (*catch_list)[2];

      ast retVal;
      try
      {
        retVal = EVAL (eval_part, a_env);
      }
      catch (const mal_exception& ex)
      {
        auto catch_env = environment::make (a_env);
        catch_env->set (bind_ex_symbol->symbol (), mal::make_string (ex.what ()));
        retVal = EVAL (catch_eval_part, catch_env);
      }
      catch (const ast_node::ptr& ex)
      {
        auto catch_env = environment::make (a_env);
        catch_env->set (bind_ex_symbol->symbol (), ex);
        retVal = EVAL (catch_eval_part, catch_env);
      }
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
      else if (symbol == "quote")
      {
        return fn_handle_quote ();
      }
      else if (symbol == "quasiquote")
      {
        ast retVal;
        std::tie (tree, a_env, retVal) = handle_quasiquote (root_list, a_env);
        if (retVal)
          return retVal;
        continue;
      }
      else if (symbol == "defmacro!")
      {
        return fn_handle_defmacro ();
      }
      else if (symbol == "macroexpand")
      {
        return fn_handle_macroexpand ();
      }
      else if (symbol == "try*")
      {
        return fn_handle_try ();
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
std::string
PRINT (ast tree)
{
  return pr_str (tree, true);
}

///////////////////////////////
std::string
rep (const std::string& line, environment::ptr env)
{
  return PRINT (EVAL ( READ (line), env));
}


///////////////////////////////
template <typename TFn>
void
execReplSafe (TFn&& fn)
{
  try
  {
    fn ();
  }
  catch (const mal_exception_stop&)
  {
    throw;
  }
  catch (const mal_exception& ex)
  {
    printline ("error: " + ex.what ());
  }
}

///////////////////////////////
void
mainRepl (environment::ptr env)
{
  const std::string prompt = "user> ";

  // repl
  try
  {
    for (;;)
    {
      execReplSafe ([&]() {
        printline (rep (readline (prompt), env));
      });
    }
  }
  catch (const mal_exception_stop&)
  {
  }

  std::cout << std::endl;
}

///////////////////////////////
int
main(int argc, char** argv)
{
  auto env = environment::make ();
  core ns (env);

  // ext
  // eval
  auto evalFn = [env] (const call_arguments& args) -> ast_node::ptr
  {
    const auto args_size = args.size ();
    if (args_size !=  1)
      raise<mal_exception_eval_invalid_arg> ();
    return EVAL (args[0], env);
  };
  env->set ("eval", std::make_shared<ast_node_callable_builtin<decltype(evalFn)>> ("eval", evalFn));

  // apply
  auto applyFn = [env] (const call_arguments& args) -> ast_node::ptr
  {
    const auto args_size = args.size ();
    if (args_size <  2)
      raise<mal_exception_eval_invalid_arg> ();

    auto callable_node = args[0]->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();

    auto args_list = mal::make_list ();
    for (size_t i = 1; i < args_size - 1; ++i)
    {
      args_list->add_child (args[i]);
    }

    auto last_list = args [args_size - 1]->as_or_throw <ast_node_container_base, mal_exception_eval_not_list> ();
    for (size_t  i = 0, e = last_list->size (); i < e; ++i)
    {
      args_list->add_child ((*last_list)[i]);
    }

    ast retVal;
    ast tree;
    environment::ptr a_env;
    std::tie (tree, a_env, retVal) = callable_node->call_tco (call_arguments {args_list.get (), 0, args_list->size ()});
    if (retVal)
      return retVal;

    return EVAL (tree, a_env);
  };
  env->set ("apply", std::make_shared<ast_node_callable_builtin<decltype(applyFn)>> ("apply", applyFn));

  // map
  auto mapFn = [env] (const call_arguments& args) -> ast_node::ptr
  {
    const auto args_size = args.size ();
    if (args_size <  2)
      raise<mal_exception_eval_invalid_arg> ();

    auto callable_node = args[0]->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();
    auto last_list = args [1]->as_or_throw <ast_node_container_base, mal_exception_eval_not_list> ();

    auto fn = [&] (ast_node::ptr v) 
    {
      auto args_list = mal::make_list ();
      args_list->add_child (v);

      ast retVal;
      ast tree;
      environment::ptr a_env;
      std::tie (tree, a_env, retVal) = callable_node->call_tco (call_arguments {args_list.get (), 0, args_list->size ()});
      if (retVal)
        return retVal;

      return EVAL (tree, a_env);
    };

    auto retVal = mal::make_list ();
    for (size_t i = 0, e = last_list->size (); i < e; ++i)
    {
      retVal->add_child (fn ((*last_list)[i]));
    }

    return retVal;
  };
  env->set ("map", std::make_shared<ast_node_callable_builtin<decltype(mapFn)>> ("map", mapFn));

  // readlineFn
  auto readlineFn = [&] (const call_arguments& args) -> ast_node::ptr
  {
    const auto args_size = args.size ();
    if (args_size !=  1)
      raise<mal_exception_eval_invalid_arg> ();

    const std::string& prompt = args [0]->as_or_throw<ast_node_string, mal_exception_eval_not_string>()->value ();

    try
    {
      return mal::make_string (readline (prompt));
    }
    catch (const mal_exception_stop&)
    {
      std::cout << std::endl;
      return ast_node::nil_node;
    }
  };

  env->set ("readline", std::make_shared<ast_node_callable_builtin<decltype(readlineFn)>> ("readline", readlineFn));

  // argv
  auto argvList = mal::make_list ();
  for (size_t i = 1; i < argc; ++i)
  {
    argvList->add_child (READ (argv [i]));
  }
  env->set ("*ARGV*", argvList);

  // MAL
  // define not function
  EVAL (READ ("(def! not (fn* (a) (if a false true)))"), env);
  // load file
  EVAL (READ ("(def! load-file (fn* (f) (eval (read-string (str \"(do \" (slurp f) \")\")))))"), env);
  // swap!
  EVAL (READ ("(defmacro! swap! (fn* [swapAtom swapFn & swapArgs] `(do (reset! ~swapAtom (~swapFn @~swapAtom ~@swapArgs) ) (deref ~swapAtom) ) ) )"), env);
  // cond
  EVAL (READ ("(defmacro! cond (fn* (& xs) (if (> (count xs) 0) (list 'if (first xs) (if (> (count xs) 1) (nth xs 1) (throw \"odd number of forms to cond\")) (cons 'cond (rest (rest xs)))))))"), env);
  // nil?
  EVAL (READ ("(def! nil? (fn* (a) (= a nil)))"), env);
  // true?
  EVAL (READ ("(def! true? (fn* (a) (= a true)))"), env);
  // false?
  EVAL (READ ("(def! false? (fn* (a) (= a false)))"), env);
  // *host-language*
  EVAL (READ ("(def! *host-language* \"AL C++\")"), env);
  // gensym
  EVAL (READ ("(def! *gensym-counter* (atom 0))"), env);
  EVAL (READ ("(def! gensym (fn* [] (symbol (str \"G__\" (swap! *gensym-counter* (fn* [x] (+ 1 x)))))))"), env);
  // or
  EVAL (READ ("(defmacro! or (fn* (& xs) (if (empty? xs) nil (if (= 1 (count xs)) (first xs) (let* (condvar (gensym)) `(let* (~condvar ~(first xs)) (if ~condvar ~condvar (or ~@(rest xs)))))))))"), env);


  if (argc < 2)
  {
    EVAL (READ ("(println (str \"Mal [\" *host-language* \"]\"))"), env);
    mainRepl (env);
  }
  else
  {
    const std::string fileName = ast_node_string (argv [1]).to_string (true);

    execReplSafe ([&]() {
      printline (rep ("(load-file " + fileName + ")", env));
    });
  }

  return 0;
}
