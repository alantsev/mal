#pragma once

#include "ast_details.h"

///////////////////////////////
class ast_builder
{
public:
  ast_builder ();

  ast_builder& open_hashmap ();
  ast_builder& close_hashmap ();

  ast_builder& open_list ();
  ast_builder& close_list ();

  ast_builder& open_vector ();
  ast_builder& close_vector ();

  ast_builder& add_symbol (std::string);
  ast_builder& add_keyword (std::string);
  ast_builder& add_int (int);

  ast_builder& add_bool (bool val);
  ast_builder& add_nil ();

  // adds complete string
  ast_builder& add_string (std::string);

  // builds string by pieces
  ast_builder& start_string ();
  ast_builder& append_string (const std::string &piece);
  ast_builder& finish_string ();

  //
  ast_builder& add_node (ast_node::ptr);

  ast build();

  //
  inline size_t level () const
  {
    assert (m_current_stack.size () > 0);
    return m_current_stack.size () - 1;
  }

private:
  std::unique_ptr<ast_node_list> m_meta_root;
  std::deque<ast_node_container_base*> m_current_stack;
  std::string m_picewise_string;
};

