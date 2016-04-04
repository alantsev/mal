#include "ast_node_builder.h"

///////////////////////////////
/// ast_builder class
///////////////////////////////
ast_builder::ast_builder ()
  : m_meta_root (new ast_node_list {})
{
  push_node (m_meta_root.get ());
}

///////////////////////////////
ast_builder& 
ast_builder::add_reader_macro (const reader_macro_fn& reader_macro)
{
  m_current_stack.back ().m_reader_macros.emplace_back (reader_macro, back_node ()->size ());
  return *this;
}
  
///////////////////////////////
ast_builder&
ast_builder::open_list ()
{
  auto child = mal::make_list ();
  ast_node_list* child_ref = child.get ();

  back_node ()->add_child (child);
  push_node (child_ref);
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::close_list ()
{
  back_node ()->as_or_throw<ast_node_list, mal_exception_parse_error> ();
  pop_node ();
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::open_vector ()
{
  auto child = mal::make_vector ();
  ast_node_vector* child_ref = child.get ();

  back_node ()->add_child (child);
  push_node (child_ref);
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::close_vector ()
{
  back_node ()->as_or_throw<ast_node_vector, mal_exception_parse_error> ();
  pop_node ();
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::add_symbol (std::string value)
{
  ast_node::ptr child { new ast_node_symbol {std::move (value)} };
  back_node ()->add_child (child);
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::add_keyword (std::string keyword)
{
  ast_node::ptr child { new ast_node_keyword {std::move (keyword)} };
  back_node ()->add_child (child);
  return *this;
}

///////////////////////////////
ast_builder& 
ast_builder::add_int (int64_t value)
{
  ast_node::ptr child { new ast_node_int { value } };
  back_node ()->add_child (child);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::add_bool (bool value)
{
  back_node ()->add_child (value ? ast_node::true_node : ast_node::false_node);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::add_nil ()
{
  back_node ()->add_child (ast_node::nil_node);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::add_node (ast_node::ptr node)
{
  back_node ()->add_child (node);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::add_string (std::string str)
{
  ast_node::ptr child { new ast_node_string {std::move (str)} };
  back_node ()->add_child (child);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::start_string ()
{
  m_picewise_string = "";
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::append_string (const std::string &piece)
{
  m_picewise_string += piece;
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::finish_string ()
{
  add_string (m_picewise_string);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::open_hashmap ()
{
  auto child = mal::make_ht_list ();
  auto child_ref = child.get ();

  back_node ()->add_child (child);
  push_node (child_ref);
  return *this;
}

///////////////////////////////
ast_builder&
ast_builder::close_hashmap ()
{
  back_node ()->as_or_throw<ast_node_ht_list, mal_exception_parse_error> ();
  if (back_node ()->size () % 2 != 0) 
    raise<mal_exception_parse_error> ("odd number of entries for hashmap: " + back_node ()->to_string ());

  pop_node ();

  return *this;
}

///////////////////////////////
ast 
ast_builder::build()
{
  if (m_current_stack.size () != 1) 
    raise<mal_exception_parse_error> ();

  apply_reader_macro ();

  const size_t level0_count = m_meta_root->size (); 
  if (level0_count > 1) 
    raise<mal_exception_parse_error> (m_meta_root->to_string ());

  if (level0_count == 0)
    return ast {};

  assert (m_meta_root->size () > 0);
  ast_node::ptr retVal = (*m_meta_root) [0];
  m_current_stack.clear ();
  m_meta_root.reset ();

  return retVal;
}

///////////////////////////////
void
ast_builder::pop_node ()
{
  apply_reader_macro ();
  m_current_stack.pop_back ();
  if (m_current_stack.size () == 0) 
    raise<mal_exception_parse_error> ();
}

///////////////////////////////
void
ast_builder::push_node (ast_node_container_base* child_ref)
{
  m_current_stack.emplace_back ();
  m_current_stack.back ().m_builder = child_ref;

}

///////////////////////////////
void
ast_builder::apply_reader_macro ()
{
  auto & entry = m_current_stack.back ();
  for (auto iter = entry.m_reader_macros.rbegin (), e = entry.m_reader_macros.rend (); iter != e; ++iter)
  {
    const auto listIdx = iter->second;
    if (listIdx >= entry.m_builder->size ())
      raise<mal_exception_parse_error> (entry.m_builder->to_string ());

    iter->first (entry.m_builder, listIdx);
  }
}


