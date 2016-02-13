#pragma once

#include "ast.h"
#include "ast_details.h"
#include "exceptions.h"
#include "environment.h"

#include <unordered_map>
#include <map>
#include <memory>

///////////////////////////////
class core
{
public:
  using symbol_lookup_map = std::unordered_map <std::string, ast_node::ptr>;
  explicit core (environment::ptr root_env);

  const symbol_lookup_map& content () const 
  {
    return m_content;
  }

private:
  //
  template <typename builtin_fn>
  void env_add_builtin (const std::string& symbol, builtin_fn fn)
  {
	m_content[symbol] = std::make_unique<ast_node_callable_builtin<builtin_fn>> (symbol, fn);
  }

  symbol_lookup_map m_content;
};
