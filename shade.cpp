#include "shade.hpp"
#include "process.hpp"
#include "d3d.hpp"
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

void Shade::win32_error(DWORD err_no, std::string message)
{
	char *msg_buffer;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |	FORMAT_MESSAGE_IGNORE_INSERTS, 0, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg_buffer, 0, 0);
	
	std::stringstream msg;

	msg << message << "\nError #" << err_no << ": " << msg_buffer;

	LocalFree(msg_buffer);

	error(msg.str());
}

void Shade::win32_error(std::string message)
{
	win32_error(GetLastError(), message);
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

void Shade::init()
{
	get_debug_privileges();
	
	find_process();
	//create_process();
	allocate_shared_memory();
	get_preset_offset();

	init_disassembler();
	compile_module();

	resume_process();
}

void Shade::remote_call(Call::Type type)
{
	shared->call_type = type;
	signal_event(local.end);
	wait_event(local.start);
	reset_event(local.start);
}

void Shade::loop(d3c_tick_t tick_func)
{
	while(true)
	{
		remote_call(Call::Continue);

		tick_func();
	}
}

extern "C" D3C_EXPORT d3c_error_t D3C_API d3c_loop(d3c_tick_t tick_func)
{
	return Shade::wrap([&] {
		Shade::loop(tick_func);
	});
}

extern "C" D3C_EXPORT d3c_error_t D3C_API d3c_init()
{
	return Shade::wrap([&] {
		Shade::init();
	});
}
