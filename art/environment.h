#pragma once

#include "ast.h"
#include "exceptions.h"

#include <unordered_map>
#include <map>
#include <memory>

///////////////////////////////
class environment
{
public:
  environment (const environment* outer = nullptr);

  template <typename TBindsVector, typename TExprsVector>
  environment (const TBindsVector& binds, const TExprsVector& exprs, const environment* outer = nullptr);

  const environment* find (const std::string &symbol) const;
  void set (const std::string& symbol, ast_node::ptr val);

  ast_node::ptr get (const std::string& symbol) const;
  ast_node::ptr get_or_throw (const std::string& symbol) const;

private:
  environment(const environment&) = delete;
  environment& operator = (const environment&) = delete;

  using symbol_lookup_map = std::unordered_map <std::string, ast_node::ptr>;
  symbol_lookup_map m_data;
  const environment* m_outer = nullptr;
};

///////////////////////////////
template <typename TBindsVector, typename TExprsVector>
environment::environment (const TBindsVector& binds, const TExprsVector& exprs, const environment* outer)
  : m_outer (outer)
{
  assert (binds.size () == exprs.size ());
  for (size_t i = 0, e = binds.size (); i < e; ++i)
  {
    auto symbol = binds[i]->template as_or_throw<ast_node_atom_symbol, mal_exception_eval_not_symbol> ()->symbol ();
    m_data[symbol] = exprs[i];
  }
}

///////////////////////////////
inline void env_add_builtin (environment& env, const std::string& symbol, ast_node_callable_builtin::builtin_fn fn)
{
  env.set (symbol, std::make_unique<ast_node_callable_builtin> (symbol, fn));
}

///////////////////////////////
ast_node::ptr builtin_plus (const call_arguments& args);
ast_node::ptr builtin_minus (const call_arguments& args);
ast_node::ptr builtin_div (const call_arguments& args);
ast_node::ptr builtin_mul (const call_arguments& args);
