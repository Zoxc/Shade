#pragma once
#include "../shade.hpp"

#include <fstream>

namespace Shade
{
	extern std::ofstream code_log;

	void init_disassembler();
	void disassemble_code(void *code, void *target, size_t size);
	void detour(void *address, void *target, void *&trampoline);
};