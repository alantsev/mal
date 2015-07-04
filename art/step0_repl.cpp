#include <iostream>
#include <string>

static const char* PROMPT = "user> ";

int
main(int, char**)
{
    std::string line;
    std::cout << PROMPT;
    while (getline(std::cin, line))
    {
        std::cout << line << std::endl;
        std::cout << PROMPT;
    }
    std::cout << std::endl;
    return 0;
}