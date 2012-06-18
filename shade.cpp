#include "shade.hpp"

LLVMContext context;

extern "C" D3C_EXPORT void D3C_API d3c_init()
{
	InitializeNativeTarget();
}
