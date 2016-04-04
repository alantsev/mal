#include "MAL.h"
#include "ast.h"
#include "ast_node_builder.h"
#include "exceptions.h"

#include <string>
#include <cctype>
#include <cstdlib>

#include <functional>
#include <deque>

///////////////////////////////
namespace
{

///////////////////////////////
class reader 
{
public:
  reader (const std::string &line)
    : m_line (line)
  {
    for (size_t e = m_line.length (); m_current_position < e;)
    {
      auto ch = m_line [m_current_position];
      ++m_current_position;
      call_op (ch);
    }

    if (m_currrent_state != state_enum::COMMENT_STATE)
    {
      if (m_currrent_state == state_enum::STRING_STATE || m_currrent_state == state_enum::ESCAPED_STRING_STATE)
        raise<mal_exception_parse_error> ("expected \" , got EOF");

      assert (m_currrent_state == state_enum::NON_EXPAND_NORMAL_STATE || m_currrent_state == state_enum::EXPAND_NORMAL_STATE);


      process_token (m_last_delim_position, m_current_position + 1);
    }
  }

  ast build ()
  {
    return builder ().build ();
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
    auto get_token = [from, to, this] ()  -> std::string
    {
      return m_line.substr (from, to - from - 1);
    };

    // TODO - check keyword first

    if (from + 1 < to)
    {
      char *end;
      int64_t valInt = std::strtoll (m_line.data () + from, &end, 10);
      if (end == m_line.data () + to - 1)
      {
        builder ().add_int (valInt);
      }
      else
      {
        const std::string token = get_token ();
        if (token[0] == ':')
        {
          builder ().add_keyword (std::move (token));
        }
        else if (token == "false")
        {
          builder ().add_bool (false);
        }
        else if (token == "true")
        {
          builder ().add_bool (true);
        }
        else if (token == "nil")
        {
          builder ().add_nil ();
        }
        else
        {
          builder ().add_symbol (std::move (token));
        }
      }
    }
  }

  void process_string (size_t from, size_t to) 
  {
    if (from + 1 < to)
    {
      builder ().append_string (m_line.substr (from, to - from - 1));
    }
  }

  //
  void mark_delimiter ()
  {
    m_last_delim_position = m_current_position;
    m_currrent_state = state_enum::EXPAND_NORMAL_STATE;
  }

  //
  void expand_normal_parse_fn (char ch)
  {
    auto make_basic_reader_macro = [] (const std::string &symbolName)
    {
      return [symbolName] (ast_node::ptr node) -> ast_node::ptr
        {
          auto retVal = mal::make_list ();
          retVal->add_child (mal::make_symbol (symbolName));
          retVal->add_child (node);
          return retVal;
        };
    };

    switch (ch)
    {
      case '@':
      {
        m_last_delim_position = m_current_position;
        builder ().add_reader_macro (make_basic_reader_macro ("deref"));
        break;
      }
      case '\'':
      {
        m_last_delim_position = m_current_position;
        builder ().add_reader_macro (make_basic_reader_macro ("quote"));
        break;
      }
      case '`':
      {
        m_last_delim_position = m_current_position;
        builder ().add_reader_macro (make_basic_reader_macro ("quasiquote"));
        break;
      }
      case '~':
      {
        if (m_current_position < m_line.length () && m_line[m_current_position] == '@')
        {
          builder ().add_reader_macro (make_basic_reader_macro ("splice-unquote"));
          ++m_current_position;
        }
        else
        {
          builder ().add_reader_macro (make_basic_reader_macro ("unquote"));
        }

        m_last_delim_position = m_current_position;
        break;
      }

      default:
      {
        m_currrent_state = state_enum::NON_EXPAND_NORMAL_STATE;
        call_op (ch);
        break;
      }
    };
  }


  //
  void non_expand_normal_parse_fn (char ch)
  {
    switch (ch)
    {
      case '(':
      {
        process_token (m_last_delim_position, m_current_position);
        builder ().open_list ();
        mark_delimiter ();
        break;
      }
      case ')':
      {
        process_token (m_last_delim_position, m_current_position);
        builder ().close_list ();
        mark_delimiter ();
        break;
      }
      case '{':
      {
        process_token (m_last_delim_position, m_current_position);
        builder ().open_hashmap ();
        mark_delimiter ();
        break;
      }
      case '}':
      {
        process_token (m_last_delim_position, m_current_position);
        builder ().close_hashmap ();
        mark_delimiter ();
        break;
      }
      case '[':
      {
        process_token (m_last_delim_position, m_current_position);
        builder ().open_vector ();
        mark_delimiter ();
        break;
      }
      case ']':
      {
        process_token (m_last_delim_position, m_current_position);
        builder ().close_vector ();
        mark_delimiter ();
        break;
      }
      case '"':
      {
        process_token (m_last_delim_position, m_current_position);
        builder ().start_string ();
        m_currrent_state = state_enum::STRING_STATE;
        m_last_delim_position = m_current_position;
        break;
      }
      case ';':
      {
        process_token (m_last_delim_position, m_current_position);
        m_currrent_state = state_enum::COMMENT_STATE;
        m_last_delim_position = m_current_position;
        break;
      }
      default:
      {
        if (std::isspace (ch) || ch == ',')
        {
          process_token (m_last_delim_position, m_current_position);
          mark_delimiter ();
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
        m_currrent_state = state_enum::EXPAND_NORMAL_STATE;
        process_string (m_last_delim_position, m_current_position);
        builder ().finish_string ();
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
        builder ().append_string ("\n");
        break;
      }
      default:
      {
        builder ().append_string ({ch});
        break;
      }
    };
    m_last_delim_position = m_current_position;
    m_currrent_state = state_enum::STRING_STATE;
  }

  void comment_parse_fn (char ch)
  {
    switch (ch)
    {
      case '\n':
      {
        m_currrent_state = state_enum::EXPAND_NORMAL_STATE;
        m_last_delim_position = m_current_position;
        break;
      }
    };
  }

  ast_builder & builder ()
  {
    return m_builder;
  }

  //
  using state_fn_type = void (reader::*) (char);

  enum class state_enum
  {
    EXPAND_NORMAL_STATE = 0,
    NON_EXPAND_NORMAL_STATE ,
    STRING_STATE,
    ESCAPED_STRING_STATE,
    COMMENT_STATE,
    COUNT
  };

  enum class operation_enum
  {
    NORMAL_OPERATION = 0,
    COUNT
  };

  static state_fn_type m_state_op [static_cast<int> (state_enum::COUNT)][static_cast<int> (operation_enum::COUNT)];

  state_enum m_currrent_state {state_enum::EXPAND_NORMAL_STATE};

  //
  const std::string &m_line;


  ast_builder m_builder;

  size_t m_last_delim_position = 0;
  size_t m_current_position = 0;
};

// static
reader::state_fn_type
reader::m_state_op [static_cast<int> (state_enum::COUNT)][static_cast<int> (operation_enum::COUNT)] = 
{
  // EXPAND_NORMAL_STATE
  {
    // NORMAL_OPERATION
    &reader::expand_normal_parse_fn
  }, 

  // NON_EXPAND_NORMAL_STATE
  {
    // NORMAL_OPERATION
    &reader::non_expand_normal_parse_fn
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
  },

  // COMMENT_STATE
  {
    // NORMAL_OPERATION
    &reader::comment_parse_fn
  }
};


} // end of the anonymous namespace


ast 
read_str (const std::string &line)
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
