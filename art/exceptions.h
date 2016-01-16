#pragma once

//
class mal_exception
{
protected:
	mal_exception () = default;
};

enum class mal_exception_enum : uint32_t
{
	PARSE_ERROR = 0,
	EVAL_ERROR_NOT_CALLABLE,
	EVAL_ERROR_NOT_INT,
	EVAL_ERROR_INVALID_ARGUMENT,
};

template <typename T> void raise ();

template <mal_exception_enum t>
class mal_exception_impl : public mal_exception
{
friend void raise<mal_exception_impl<t>> ();
private:
	mal_exception_impl () = default;
};

using mal_exception_parse_error = mal_exception_impl<mal_exception_enum::PARSE_ERROR>;
using mal_exception_eval_not_callable = mal_exception_impl<mal_exception_enum::EVAL_ERROR_NOT_CALLABLE>;
using mal_exception_eval_not_int = mal_exception_impl<mal_exception_enum::EVAL_ERROR_NOT_INT>;
using mal_exception_eval_invalid_arg = mal_exception_impl<mal_exception_enum::EVAL_ERROR_INVALID_ARGUMENT>;

template <typename T>
void raise ()
{
	throw T ();
}
