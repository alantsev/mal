#include <iostream>
#include <string>

static const char* PROMPT = "user> ";

//
class repl_exception{};

//
std::string readline()
{
    std::string line;
    if (!getline(std::cin, line))
        throw repl_exception();
    return line;
};

//
std::string
READ()
{
    return readline();
}

//
std::string
EVAL(const std::string& input)
{
    return input;
}

//
std::string
PRINT(const std::string& input)
{
    return input;
}

//
std::string
rep()
{
    auto&& line = READ();
    auto&& ast = EVAL(line);
    auto&& output = PRINT(ast);
    return output;
}

//
void repl()
{
    try
    {
        for (;;)
        {
            std::cout << PROMPT;
            auto&& output = rep();
            std::cout << output << std::endl;
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