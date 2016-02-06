#pragma once

#include "ast.h"

ast read_str (std::string line);
std::string pr_str (ast a_ast, bool print_readably);
