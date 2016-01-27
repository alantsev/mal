#pragma once

#include <string>

///////////////////////////////
enum class mal_exception_enum : uint32_t
{
  PARSE_ERROR = 0,
  EVAL_ERROR_NOT_CALLABLE,
  EVAL_ERROR_NOT_INT,
  EVAL_ERROR_NOT_LIST,
  EVAL_ERROR_NOT_SYMBOL,
  EVAL_ERROR_INVALID_ARGUMENT,
  EVAL_ERROR_NO_SYMBOL,
  STOP
};

///////////////////////////////
template <typename T> void raise (std::string message = "");

///////////////////////////////
template <mal_exception_enum t>
class mal_exception_impl
{
friend void raise<mal_exception_impl<t>> (std::string message);
public:
  const std::string& what () const 
  {
    return m_message;
  }
private:
  mal_exception_impl (std::string message = "")
    : m_message (std::move (message))
  {}

  std::string m_message;
};

///////////////////////////////
using mal_exception_parse_error = mal_exception_impl<mal_exception_enum::PARSE_ERROR>;
using mal_exception_eval_not_callable = mal_exception_impl<mal_exception_enum::EVAL_ERROR_NOT_CALLABLE>;
using mal_exception_eval_not_int = mal_exception_impl<mal_exception_enum::EVAL_ERROR_NOT_INT>;
using mal_exception_eval_not_list = mal_exception_impl<mal_exception_enum::EVAL_ERROR_NOT_LIST>;
using mal_exception_eval_not_symbol = mal_exception_impl<mal_exception_enum::EVAL_ERROR_NOT_SYMBOL>;
using mal_exception_eval_invalid_arg = mal_exception_impl<mal_exception_enum::EVAL_ERROR_INVALID_ARGUMENT>;
using mal_exception_eval_no_symbol = mal_exception_impl<mal_exception_enum::EVAL_ERROR_NO_SYMBOL>;
using mal_exception_stop = mal_exception_impl<mal_exception_enum::STOP>;

///////////////////////////////
template <typename T>
void raise (std::string message)
{
  throw T (std::move (message));
}
