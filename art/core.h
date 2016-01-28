#pragma once

#include "ast.h"
#include "exceptions.h"

#include <unordered_map>
#include <map>
#include <memory>

///////////////////////////////
class core
{
public:
  using symbol_lookup_map = std::unordered_map <std::string, ast_node::ptr>;
  core ();

  const symbol_lookup_map& content () const 
  {
    return m_content;
  }

private:
  //
  void env_add_builtin (const std::string& symbol, ast_node_callable_builtin::builtin_fn fn);
  symbol_lookup_map m_content;
};
