#include "d3.hpp"

namespace Shade
{
	D3::Module D3::diablo_exe;
	
	void D3::init()
	{
		diablo_exe.delta = (size_t)GetModuleHandle(0) - 0x800000;
	}
};
