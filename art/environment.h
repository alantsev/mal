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

	const environment* find (const std::string &symbol) const;
	const ast_node* set (const std::string& symbol, std::unique_ptr<ast_node> val);

	const ast_node* get (const std::string& symbol) const;
	const ast_node* get_or_throw (const std::string& symbol) const;

private:
	environment(const environment&) = delete;
	environment& operator = (const environment&) = delete;

	using symbol_lookup_map = std::unordered_map <std::string, std::unique_ptr<ast_node>>;
	symbol_lookup_map m_data;
	const environment* m_outer = nullptr;
};

inline void env_add_builtin (environment& env, const std::string& symbol, ast_node_callable_builtin::builtin_fn fn)
{
	env.set (symbol, std::make_unique<ast_node_callable_builtin> (symbol, fn));
}

ast_node::ptr builtin_plus (const call_arguments& args);
ast_node::ptr builtin_minus (const call_arguments& args);
ast_node::ptr builtin_div (const call_arguments& args);
ast_node::ptr builtin_mul (const call_arguments& args);
