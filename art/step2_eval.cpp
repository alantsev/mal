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
eval_ast (ast_node::ptr node, const environment& a_env); // fwd decl

///////////////////////////////
ast_node::ptr
eval_impl (ast_node::ptr root, const environment& a_env)
{
    if (root->type () != node_type_enum::LIST)
        return { eval_ast (std::move (root), a_env) };

    ast_node::ptr new_node = eval_ast (std::move (root), a_env);
    auto new_node_list = new_node->as_or_throw<ast_node_list, mal_exception_eval_not_list> ();

    return apply (new_node_list);
}


///////////////////////////////
ast_node::ptr
eval_ast (ast_node::ptr node, const environment& a_env)
{
    switch (node->type ())
    {
    case node_type_enum::SYMBOL:
        {
            // as_or_throw ?
            const auto& node_symbol = node->as<ast_node_atom_symbol> ();
            return {a_env.symbol_lookup_or_throw (node_symbol->symbol ())->clone ()};

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
EVAL (ast a_ast, const environment& a_env)
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
rep ()
{
    static environment repl_env;
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
