#include "MAL.h"
#include "ast.h"
#include "ast_details.h"

///////////////////////////////
ast_node::ptr ast_node::nil_node {new ast_node_nil {}};
ast_node::ptr ast_node::true_node {new ast_node_bool<true> {}};
ast_node::ptr ast_node::false_node {new ast_node_bool<false> {}};

