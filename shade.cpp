#include "shade.hpp"
#include "compiler.hpp"
#include "disassembler.hpp"

d3c_error_t Shade::error(d3c_error_type_t error, const char *message)
{
	auto result = new d3c_error;

	result->num = error;
	result->message = strdup(message);

	return result;
}

extern "C" D3C_EXPORT void D3C_API d3c_free_error(d3c_error_t error)
{
	if(error->message)
		free((void *)error->message);

	delete error;
}

extern "C" D3C_EXPORT d3c_error_t D3C_API d3c_init()
{
	try
	{
		Shade::init_disassembler();
		Shade::compile_module();

		return 0;
	} catch(d3c_error_t error)
	{
		
		return error;
	}
}
