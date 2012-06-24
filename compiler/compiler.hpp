#pragma once
#include "../shade.hpp"
#include "remote-heap.hpp"

namespace Shade
{
	extern RemoteHeap code_section;

	void compile_module();
};