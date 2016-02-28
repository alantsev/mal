#pragma once

#include <memory>

///////////////////////////////
#define UNUSED(x) ((void) x)

///////////////////////////////
class environment;

///////////////////////////////
class ast_node;

class ast_node_symbol;
class ast_node_int;
class ast_node_atom;

class ast_node_container_base;
class ast_node_list;
class ast_node_vector;
class ast_node_hashmap;

class call_arguments;

class ast_node_callable;

template <typename builtin_fn>
class ast_node_callable_builtin;
class ast_node_callable_lambda;
