#pragma once

#include "ast.h"
#include "exceptions.h"

#include <map>
#include <memory>

///////////////////////////////
class environment
{
public:
	environment ();

	const ast_node* symbol_lookup (const std::string &symbol) const noexcept;

	// throws mal_exception_eval_no_symbol
	const ast_node* symbol_lookup_or_throw (const std::string &symbol) const;

private:
	inline void env_add_builtin (const std::string& symbol, ast_node_callable_builtin::builtin_fn fn)
	{
	    m_env.insert (std::make_tuple (symbol, std::make_unique<ast_node_callable_builtin> (symbol, fn)));
	}

	using symbol_lookup_map = std::map <std::string, std::unique_ptr<ast_node>>;
	symbol_lookup_map m_env;
};