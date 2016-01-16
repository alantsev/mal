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
    {
        char * line = readline(PROMPT);
        if (!line)
            raise<mal_exception_parse_error> ();

        if (*line) 
            add_history(line);

        retVal = line;
        free(line);
    }
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
EVAL (ast &&a_ast)
{
    return std::move(a_ast);
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
    PRINT (EVAL ( READ()));
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
        }
    }
    catch (const mal_exception&)
    {
    }
    std::cout << std::endl;
}
