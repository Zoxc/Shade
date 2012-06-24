#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Shade
{
	namespace Remote
	{
		typedef unsigned int size_t;

		size_t init();
	};
};
