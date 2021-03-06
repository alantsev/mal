#pragma once

#include "ast.h"

ast read_str (const std::string &line);
std::string pr_str (ast a_ast, bool print_readably);

std::string readline (const std::string& prompt);
