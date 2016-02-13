#pragma once

#include <memory>

///////////////////////////////
class environment;
using environment_ptr = std::shared_ptr <environment>;

///////////////////////////////
class ast_node;

class ast_node_atom_symbol;
class ast_node_atom_int;

class ast_node_container_base;
class ast_node_list;
class ast_node_vector;
class ast_node_hashmap;

class call_arguments;

class ast_node_callable;

template <typename builtin_fn>
class ast_node_callable_builtin;
class ast_node_callable_lambda;
