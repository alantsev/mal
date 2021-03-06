#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>
#include <string>

#include "MAL.h"
#include "exceptions.h"

static const char* PROMPT = "user> ";

//
std::string 
readline()
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

void
printline(std::string&& line)
{
    std::cout << line << std::endl;
};


//
std::string
READ()
{
    return readline();
}

//
std::string
EVAL(std::string&& input)
{
    return std::move(input);
}

//
void
PRINT(std::string&& input)
{
    printline(std::move(input));
}

//
void
rep()
{
    auto&& line = READ();
    auto&& ast = EVAL(std::move(line));
    PRINT(std::move(ast));
}

//
int
main(int, char**)
{
    try
    {
        for (;;)
        {
            rep();
        }
    }
    catch (const mal_exception_stop&)
    {
    }
    std::cout << std::endl;
}