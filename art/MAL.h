#ifndef _MAL_H_
#define _MAL_H_

#include "ast.h"
#include <assert.h>

//
class repl_exception
{};

//
class parse_error 
{};

ast read_str (std::string line);
std::string pr_str (ast a_ast);

#endif
