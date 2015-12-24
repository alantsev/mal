#include "MAL.h"
#include "ast.h"

#include <string>
#include <cctype>
#include <cstdlib>

ast 
read_str (std::string line)
{
  ast_builder builder;

  size_t last_delim_position = 0;
  size_t current_position = 0;

  auto fnProcessToken = [&line, &builder] (size_t from, size_t to) 
  {
    if (from + 1 < to)
    {
      char *end;
      int valInt = std::strtol(line.data () + from, &end, 10);

      if (end == line.data () + to)
      {
        builder.add_int (valInt);
      }
      else
      {
        builder.add_symbol (line.substr (from, to - from - 1));
      }
    }
  };

  for (auto ch : line)
  {
    ++current_position;
    if (ch == '(')
    {
      fnProcessToken (last_delim_position, current_position);
      last_delim_position = current_position;
      builder.open_list ();
    }
    else if (ch == ')')
    {
      fnProcessToken (last_delim_position, current_position);
      last_delim_position = current_position;
      builder.close_list ();
    }
    else if (std::isspace (ch))
    {
      fnProcessToken (last_delim_position, current_position);
      last_delim_position = current_position;
    }
  }
  fnProcessToken (last_delim_position, current_position + 1);

  return builder.build ();

}

std::string 
pr_str (ast a_ast)
{
  if (!a_ast.m_root)
    return {};
  return a_ast.m_root->to_string ();
}
