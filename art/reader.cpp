#include "MAL.h"
#include "ast.h"
#include "ast_details.h"
#include "exceptions.h"

#include <string>
#include <cctype>
#include <cstdlib>

#include <functional>

///////////////////////////////
namespace
{

///////////////////////////////
class reader 
{
public:
  reader (std::string line)
    : m_line (std::move (line))
  {
    for (auto ch : m_line)
    {
      ++m_current_position;
      call_op (ch);
    }
    if (m_currrent_state != state_enum::NORMAL_STATE)
      raise<mal_exception_parse_error> ("expected \" , got EOF");

    process_token (m_last_delim_position, m_current_position + 1);
  }

  ast build ()
  {
    return m_builder.build ();
  }

private:
  //
  void call_op (char ch)
  {
    auto fn = reader::m_state_op [static_cast<int> (m_currrent_state)]
                                 [static_cast<int> (operation_enum::NORMAL_OPERATION)];
    (this->*fn)(ch);
  }


  //
  void process_token (size_t from, size_t to) 
  {
    if (from + 1 < to)
    {
      char *end;
      int valInt = std::strtol (m_line.data () + from, &end, 10);
      if (end == m_line.data () + to - 1)
      {
        m_builder.add_int (valInt);
      }
      else
      {
        std::string token = m_line.substr (from, to - from - 1);
        if (token == "false")
        {
          m_builder.add_bool (false);
        }
        else if (token == "true")
        {
          m_builder.add_bool (true);
        }
        else if (token == "nil")
        {
          m_builder.add_nil ();
        }
        else
        {
          m_builder.add_symbol (std::move (token));
        }
      }
    }
  }

  void process_string (size_t from, size_t to) 
  {
    if (from + 1 < to)
    {
      m_builder.append_string (m_line.substr (from, to - from - 1));
    }
  }

  //
  void normal_parse_fn (char ch)
  {
    switch (ch)
    {
      case '(':
      {
        process_token (m_last_delim_position, m_current_position);
        m_last_delim_position = m_current_position;
        m_builder.open_list ();
        break;
      }
      case ')':
      {
        process_token (m_last_delim_position, m_current_position);
        m_last_delim_position = m_current_position;
        m_builder.close_list ();
        break;
      }
      case '[':
      {
        process_token (m_last_delim_position, m_current_position);
        m_last_delim_position = m_current_position;
        m_builder.open_vector ();
        break;
      }
      case ']':
      {
        process_token (m_last_delim_position, m_current_position);
        m_last_delim_position = m_current_position;
        m_builder.close_vector ();
        break;
      }
      case '"':
      {
        m_currrent_state = state_enum::STRING_STATE;
        process_token (m_last_delim_position, m_current_position);
        m_builder.start_string ();

        m_last_delim_position = m_current_position;
        break;
      }

      default:
      {
        if (std::isspace (ch) || ch == ',')
        {
          process_token (m_last_delim_position, m_current_position);
          m_last_delim_position = m_current_position;
        }
        break;
      }
    };
  }

  //
  void string_parse_fn (char ch)
  {
    switch (ch)
    {
      case '"':
      {
        m_currrent_state = state_enum::NORMAL_STATE;
        process_string (m_last_delim_position, m_current_position);
        m_builder.finish_string ();
        m_last_delim_position = m_current_position;
        break;
      }
      case '\\':
      {
        process_string (m_last_delim_position, m_current_position);
        m_last_delim_position = m_current_position;
        m_currrent_state = state_enum::ESCAPED_STRING_STATE;
        break;
      }

    };
  }

  void escaped_string_parse_fn (char ch)
  {
    switch (ch)
    {
      case 'n':
      {
        m_builder.append_string ("\n");
        break;
      }
      default:
      {
        m_builder.append_string ({ch, 0});
        break;
      }
    };
    m_last_delim_position = m_current_position;
    m_currrent_state = state_enum::STRING_STATE;
  }

  //
  using state_fn_type = void (reader::*) (char);

  enum class state_enum
  {
    NORMAL_STATE = 0,
    STRING_STATE,
    ESCAPED_STRING_STATE,
    COUNT
  };

  enum class operation_enum
  {
    NORMAL_OPERATION = 0,
    COUNT
  };

  static state_fn_type m_state_op [static_cast<int> (state_enum::COUNT)][static_cast<int> (operation_enum::COUNT)];

  state_enum m_currrent_state {state_enum::NORMAL_STATE};

  //
  const std::string m_line;
  ast_builder m_builder;

  size_t m_last_delim_position = 0;
  size_t m_current_position = 0;
};

// static
reader::state_fn_type
reader::m_state_op [static_cast<int> (state_enum::COUNT)][static_cast<int> (operation_enum::COUNT)] = 
{
  // NORMAL_STATE
  {
    // NORMAL_OPERATION
    &reader::normal_parse_fn
  }, 

  // STRING_STATE
  {
    // NORMAL_OPERATION
    &reader::string_parse_fn
  },

  // ESCAPED_STRING_STATE
  {
    // NORMAL_OPERATION
    &reader::escaped_string_parse_fn
  }

};


} // end of the anonymous namespace


ast 
read_str (std::string line)
{
  return reader {line}.build ();
}

std::string 
pr_str (ast tree, bool print_readably)
{
  if (!tree)
    return {};
  return tree->to_string (print_readably);
}
