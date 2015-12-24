#include "MAL.h"
#include "ast.h"

#include <string>
#include <cctype>

ast 
read_str (std::string line)
{
	ast_builder builder;
/*
	size_t last_delim_position = 0;
	size_t current_position = 0;

	for (auto ch : line)
	{
		++current_position;
		if (ch == '(')
		{
			last_delim_position = current_position;
			builder.open_list ();
		}
		else if (ch == ')')
		{
			last_delim_position = current_position;
			builder.close_list ();
		}
		else if (std::isspace (ch))
		{
			last_delim_position = current_position;
		}

	}
	// FIXME - 
*/
	builder.open_list ();
	builder.add_symbol ("s1");
	builder.add_symbol ("s2");
	builder.add_int (5);

	builder.open_list ();
	builder.add_symbol ("s1");
	builder.add_symbol ("s2");
	builder.add_int (5);
	builder.close_list ();

	builder.close_list ();

	return builder.build ();

}

std::string 
pr_str (ast a_ast)
{
	if (!a_ast.m_root)
		return {};
	return a_ast.m_root->to_string ();
}
