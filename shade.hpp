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

#define D3C_EXPORTS
#include "d3c.h"

namespace Shade
{
	extern HANDLE process;
	
	void write(void *remote, const void *local, size_t size);
	void read(const void *remote, void *local, size_t size);
	
	void win32_error(std::string message);
	d3c_error_t error(std::string message);

	#define LLVM_ERROR(expr) do { auto error = (expr); if(error.value()) throw Shade::error(std::string("LLVM Error: ") + error.message().c_str()); } while(0)
};
