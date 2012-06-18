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

	d3c_init();

	return 0;
}

