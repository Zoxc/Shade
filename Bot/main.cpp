#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <iostream>
#include <d3c.h>
#include <tchar.h>

int _tmain(int argc, _TCHAR* argv[])
{
	std::cout << "Hi" << std::endl;

	auto error = d3c_init();

	if(error)
		std::cerr << "Error: " << error->message << std::endl;

	return 0;
}

