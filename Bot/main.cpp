#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <iostream>
#include <d3c.h>
#include <tchar.h>

void D3C_API tick()
{
	std::cout << "Tick" << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	std::cout << "Hi" << std::endl;

	auto error = d3c_init();

	if(error)
	{
		std::cerr << "Error: " << error->message << std::endl;
		return 1;
	}

	error = d3c_loop(&tick);
	
	if(error)
	{
		std::cerr << "Error: " << error->message << std::endl;
		return 1;
	}

	return 0;
}

