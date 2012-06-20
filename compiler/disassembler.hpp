#pragma once

#include "../shade.hpp"

namespace Shade
{
	void init_disassembler();
	void disassemble_code(void *code, void *target, size_t size);
};