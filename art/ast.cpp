#include "MAL.h"
#include "ast.h"
#include "ast_details.h"

///////////////////////////////
ast_node::ptr ast_node::nil_node = std::make_shared<ast_node_nil> ();
ast_node::ptr ast_node::true_node = std::make_shared<ast_node_bool<true>> ();
ast_node::ptr ast_node::false_node = std::make_shared<ast_node_bool<false>> ();
ast_node::ptr ast_node::invalid_node = std::make_shared<ast_node_invalid> ();
