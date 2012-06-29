#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>

namespace Shade
{
	namespace Error
	{
		enum Type;
	};

	namespace Remote
	{
		void set_error(Error::Type error);
		size_t init();
	};
};
