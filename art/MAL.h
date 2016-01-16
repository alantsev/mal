#pragma once

#include "ast.h"
#include "exceptions.h"
#include <assert.h>

ast read_str (std::string line);
std::string pr_str (ast a_ast);
