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

#define D3C_EXPORTS
#include "d3c.h"

namespace Shade
{
	d3c_error_t error(d3c_error_type_t error, const char *message);

	#define LLVM_ERROR(expr) do { auto error = (expr); if(error.value()) throw Shade::error(D3C_LLVM, error.message().c_str()); } while(0)
};
