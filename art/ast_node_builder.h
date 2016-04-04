#pragma once

#include "ast_details.h"

///////////////////////////////
class ast_builder
{
public:
  //
  using reader_macro_fn = std::function <ast_node::ptr (ast_node::ptr)>;

  //
  ast_builder ();

  ast_builder& add_reader_macro (const reader_macro_fn &);

  ast_builder& open_hashmap ();
  ast_builder& close_hashmap ();

  ast_builder& open_list ();
  ast_builder& close_list ();

  ast_builder& open_vector ();
  ast_builder& close_vector ();

  ast_builder& add_symbol (std::string);
  ast_builder& add_keyword (std::string);
  ast_builder& add_int (int64_t);

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
  //
  ast_node_container_base* back_node ()
  {
    return m_current_stack.back ().m_builder;
  }

  void push_node (ast_node_container_base*);
  void pop_node ();
  void apply_reader_macro ();

  struct builder_stack_entry
  {
    ast_node_container_base* m_builder = {};
    std::deque <std::pair<reader_macro_fn, size_t>> m_reader_macros = {};
  };

  std::unique_ptr<ast_node_list> m_meta_root;
  std::deque<builder_stack_entry> m_current_stack;

  std::string m_picewise_string;
};

