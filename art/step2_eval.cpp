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

static const char* PROMPT = "user> ";

///////////////////////////////
std::string 
readline (const std::string& prompt = PROMPT)
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
ast
apply (const ast_node_list* callable_list)
{
    const size_t list_size = callable_list->size ();
    if (list_size == 0)
        raise<mal_exception_eval_not_callable> ();

    auto && callable_node = (*callable_list)[0]->as_or_throw<ast_node_callable, mal_exception_eval_not_callable> ();

    return std::get<2> (callable_node->call_tco (call_arguments (callable_list, 1, list_size - 1)));
}

///////////////////////////////
ast
eval_ast (ast node, environment::ptr a_env); // fwd decl

///////////////////////////////
ast
EVAL (ast tree, environment::ptr a_env)
{
    if (tree->type () != node_type_enum::LIST)
        return { eval_ast (tree, a_env) };

    auto root_list = tree->as<ast_node_list> ();

    if (root_list->size () == 0)
    {
        return tree;
    }

    ast new_node = eval_ast (tree, a_env);
    auto new_node_list = new_node->as_or_throw<ast_node_list, mal_exception_eval_not_list> ();

    return apply (new_node_list);
}


///////////////////////////////
ast
eval_ast (ast node, environment::ptr a_env)
{
    switch (node->type ())
    {
    case node_type_enum::SYMBOL:
        {
            // as_or_throw ?
            const auto& node_symbol = node->as<ast_node_symbol> ();
            return a_env->get_or_throw (node_symbol->symbol ());
        }
    case node_type_enum::LIST:
        {
            // as_or_throw ?
            auto&& node_list = node->as<ast_node_list> ();
            return node_list->map ([&a_env] (ast_node::ptr v) { return EVAL (v, a_env);});
        }
    case node_type_enum::VECTOR:
        {
            // as_or_throw ?
            auto&& node_vector = node->as<ast_node_vector> ();
            return node_vector->map ([&a_env] (ast_node::ptr v) { return EVAL (v, a_env);});
        }
    case node_type_enum::HASHMAP:
        {
            // not as_or_throw - we know the type
            const auto& node_hashmap = node->as<ast_node_hashmap> ();
            auto retVal = mal::make_hashmap ();
            node_hashmap->for_each ([&] (ast_node::ptr k, ast_node::ptr v) { retVal->insert (EVAL (k, a_env), EVAL (v, a_env)); });
            return static_cast<ast_node::ptr> (retVal);
        }

    default:
        break;
    }

    return node;
}

///////////////////////////////
void
PRINT (ast tree)
{
    printline (pr_str (tree, true));
}

///////////////////////////////
void
rep ()
{
    static auto repl_env = environment::make ();
    static core ns (repl_env);

    PRINT (EVAL ( READ (), repl_env));
}

///////////////////////////////
int
main(int, char**)
{
    try
    {
        for (;;)
        {
            try
            {
                rep();
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
