#pragma once

#include "MAL.h"
#include "ast.h"
#include "exceptions.h"

#include <unordered_map>
#include <map>
#include <memory>

///////////////////////////////
class environment : public std::enable_shared_from_this<environment>
{
private:
    struct hide_me {};

public:

  using ptr = std::shared_ptr<environment>;
  using const_ptr = std::shared_ptr<const environment>;
  //
  environment (hide_me, environment::const_ptr outer);
  environment (hide_me, const ast_node_container_base& binds, const call_arguments& exprs, environment::const_ptr outer);


  //
  environment::const_ptr find (const std::string &symbol) const;
  void set (const std::string& symbol, ast_node::ptr val);

  ast_node::ptr get (const std::string& symbol) const;
  ast_node::ptr get_or_throw (const std::string& symbol) const;

  //
  static environment::ptr make (environment::const_ptr outer = nullptr)
  {
    return std::make_shared<environment> (hide_me{}, outer);
  }

  static environment::ptr make (const ast_node_container_base& binds, const call_arguments& exprs, environment::const_ptr outer = nullptr)
  {
    return std::make_shared<environment> (hide_me{}, binds, exprs, outer);
  }

private:
  environment(const environment&) = delete;
  environment& operator = (const environment&) = delete;

  using symbol_lookup_map = std::unordered_map <std::string, ast_node::ptr>;
  symbol_lookup_map m_data;
  environment::const_ptr m_outer;

};
