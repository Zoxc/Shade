#pragma once
#pragma warning(disable:4996)
#pragma warning(disable:4355)
#pragma warning(disable:4275)
#pragma warning(disable:4624)

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdlib>
#include <string>
#include <Prelude/Internal/Common.hpp>

#define D3C_EXPORTS
#include "d3c.h"

#include "external/shared.hpp"

namespace Shade
{
	extern HANDLE thread;
	extern HANDLE process;
	
	void write(void *remote, const void *local, size_t size);
	void read(const void *remote, void *local, size_t size);
	
	prelude_noreturn void win32_error(std::string message);
	prelude_noreturn void error(std::string message);

	#define LLVM_ERROR(expr) do { auto error = (expr); if(error.value()) Shade::error(std::string("LLVM Error: ") + error.message().c_str()); } while(0)
	
	template<typename F> void access(void *memory, size_t size, F func)
	{
		DWORD old;

		if(!VirtualProtectEx(process, memory, size, PAGE_EXECUTE_READWRITE, &old))
			win32_error("Unable to access remote memory");

		func();
		
		if(!VirtualProtectEx(process, memory, size, old, &old))
			win32_error("Unable to restore remote memory access");
	};

	template<typename F> d3c_error_t wrap(F func)
	{
		try
		{
			func();

			return 0;
		} catch(d3c_error_t error)
		{
			return error;
		}
	}
	
	void remote_call(Call::Type type);
	void init();
	void loop(d3c_tick_t tick_func);
};
