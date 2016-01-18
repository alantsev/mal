#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>
#include <string>

#include "MAL.h"

static const char* PROMPT = "user> ";

///////////////////////////////
std::string 
readline ()
{
    std::string retVal;
    do
    {
        char * line = readline(PROMPT);
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
printline (std::string &&line)
{
    std::cout << line << std::endl;
};

///////////////////////////////
ast
READ ()
{
    return read_str ( readline ());
}

///////////////////////////////
ast_node::ptr
apply (const ast_node_list* callable_list)
{
    const size_t list_size = callable_list->size ();
    if (list_size == 0)
        raise<mal_exception_eval_not_callable> ();

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

    // as_or_throw
    auto root_list = root->as<ast_node_list> ();

    // apply special symbols
    if (root_list->size () > 0)
    {
        auto first = (*root_list)[0];
        if (first->type () == node_type_enum::SYMBOL)
        {
            // as_or_throw
            const auto first_symbol = first->as<ast_node_atom_symbol> ();
            const auto& symbol = first_symbol->symbol ();
            if (symbol == "def!")
            {
                if (root_list->size () != 3)
                    raise<mal_exception_eval_invalid_arg> ();

                const auto& key = (*root_list)[1]->as_or_throw<ast_node_atom_symbol, mal_exception_eval_invalid_arg> ()->symbol ();
                // TODO - do we really need clone here???
                ast_node::ptr value = eval_impl ((*root_list)[2]->clone (), a_env);

                // TODO - get rid of clone here!!!
                return a_env.set (key, std::move (value))->clone ();
            }
            else if (symbol == "let*")
            {
                if (root_list->size () != 3)
                    raise<mal_exception_eval_invalid_arg> ();

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
                    raise<mal_exception_eval_invalid_arg> ();
                };

                //
                environment let_env (std::addressof (a_env));

                if (let_bindings->size () % 2 != 0)
                    raise<mal_exception_eval_invalid_arg> ();
                
                for (size_t i = 0, e = let_bindings->size(); i < e; i += 2)
                {
                    const auto& key = (*let_bindings)[i]->as_or_throw<ast_node_atom_symbol, mal_exception_eval_invalid_arg> ()->symbol ();
                    // TODO - do we really need clone here???
                    ast_node::ptr value = eval_impl ((*let_bindings)[i + 1]->clone (), let_env);

                    let_env.set (key, std::move (value));
                }

                // TODO - get rid of clone here
                return eval_impl ((*root_list)[2]->clone (), let_env);
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
    switch (node->type ())
    {
    case node_type_enum::SYMBOL:
        {
            // as_or_throw ?
            const auto& node_symbol = node->as<ast_node_atom_symbol> ();
            return a_env.get_or_throw (node_symbol->symbol ())->clone ();

        }
    case node_type_enum::LIST:
        {
            // as_or_throw ?
            auto&& node_list = node->as<ast_node_list> ();
            node_list->map ([&a_env] (ast_node::ptr v) { return std::move (eval_impl (std::move (v), a_env));});
            break;
        }
    case node_type_enum::VECTOR:
        {
            // as_or_throw ?
            auto&& node_vector = node->as<ast_node_vector> ();
            node_vector->map ([&a_env] (ast_node::ptr v) { return std::move (eval_impl (std::move (v), a_env));});
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
rep (environment& env)
{
    PRINT (EVAL ( READ (), env));
}

///////////////////////////////
int
main(int, char**)
{
    try
    {
        environment env;
        env_add_builtin (env, "+", builtin_plus);
        env_add_builtin (env, "-", builtin_minus);
        env_add_builtin (env, "*", builtin_mul);
        env_add_builtin (env, "/", builtin_div);

        for (;;)
        {
            try
            {
                rep (env);
            }
            catch (const mal_exception_parse_error &)
            {
                printline ("parsing error!");
            }
            catch (const mal_exception_eval_not_callable &)
            {
                printline ("not callable!");
            }
            catch (const mal_exception_eval_not_int &)
            {
                printline ("not int!");
            }
            catch (const mal_exception_eval_not_list &)
            {
                printline ("not list!");
            }
            catch (const mal_exception_eval_invalid_arg &)
            {
                printline ("invalid arg!");
            }
            catch (const mal_exception_eval_no_symbol &)
            {
                printline ("no symbol!");
            }
        }
    }
    catch (const mal_exception_stop&)
    {
    }
    std::cout << std::endl;
}
