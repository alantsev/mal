#include "ast.h"


ast::ast(const std::string& a_line)
{
    // FIXME
    std::unique_ptr<ast_node_list> tmp_list(new ast_node_list());
    tmp_list->add_node(std::unique_ptr<ast_node>(new ast_atom_int(23)));
    tmp_list->add_node(std::unique_ptr<ast_node>(new ast_atom_int(24)));

    std::unique_ptr<ast_node_list> tmp_list1(new ast_node_list());
    tmp_list1->add_node(std::unique_ptr<ast_node>(new ast_atom_symbol("123")));
    tmp_list1->add_node(std::unique_ptr<ast_node>(new ast_atom_int(23)));
    tmp_list1->add_node(std::unique_ptr<ast_node>(new ast_atom_int(24)));

    tmp_list->add_node(std::move(tmp_list1));


    root = std::move(tmp_list);
}
