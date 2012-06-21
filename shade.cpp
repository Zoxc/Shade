#include "shade.hpp"
#include "process.hpp"
#include "compiler/compiler.hpp"
#include "compiler/disassembler.hpp"

#include <sstream>

void Shade::write(void *remote, const void *local, size_t size)
{
	if(!WriteProcessMemory(process, remote, local, size, 0))
		win32_error("Unable to write remote memory");
}

void Shade::read(const void *remote, void *local, size_t size)
{
	if(!ReadProcessMemory(process, remote, local, size, 0))
		win32_error("Unable to read remote memory");
}

void Shade::win32_error(std::string message)
{
	char *msg_buffer;
	DWORD err_no = GetLastError(); 

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |	FORMAT_MESSAGE_IGNORE_INSERTS, 0, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg_buffer, 0, 0);
	
	std::stringstream msg;

	msg << message << "\nError #" << err_no << ": " << msg_buffer;

	LocalFree(msg_buffer);

	error(msg.str());
}

void Shade::error(std::string message)
{
	auto result = new d3c_error;

	result->message = strdup(message.c_str());

	throw result;
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
		Shade::create_process();
		Shade::allocate_shared_memory();

		Shade::init_disassembler();
		Shade::compile_module();

		return 0;
	} catch(d3c_error_t error)
	{
		
		return error;
	}
}
