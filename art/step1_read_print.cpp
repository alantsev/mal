#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "ast.h"

static const char* PROMPT = "user> ";

//
class repl_exception{};

//
std::string
readline()
{
    std::string line;
    if (!getline(std::cin, line))
        throw repl_exception();
    return line;
};

void
printline(const std::string& line)
{
    std::cout << line << std::endl;
};

//
std::unique_ptr<ast>
READ()
{
    auto&& line = readline();
    return ast::parse(line);
}

//
std::unique_ptr<ast>
EVAL(std::unique_ptr<ast> ast)
{
    return std::move(ast);
}

//
void
PRINT(const ast* ast)
{
    printline(ast->to_string());
}

//
void
rep()
{
    auto&& ast = EVAL(READ());
    PRINT(ast.get());
}

//
void repl()
{
    try
    {
        for (;;)
        {
            std::cout << PROMPT;
            rep();
        }
    }
    catch (const repl_exception&)
    {
    }
    std::cout << std::endl;
}

//
int
main(int, char**)
{
    repl();
    return 0;
}