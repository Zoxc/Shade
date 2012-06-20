#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

struct A
{
	int test;
	A() {
		test = GetTickCount();
	}
	
};

A hm;

int a = 1;
int b;

extern "C" void init(int arg)
{
const char *str = "hm";
	switch(arg)
	{
	case 0: str = "v0"; break;
	case 1: str = "v1"; break;
	case 2: str = "v2"; break;
	case 3: str = "v3"; break;
	case 4: str = "v4"; break;
	case 5: str = "v5"; break;
	case 6: str = "v6"; break;
	default:
	break;
	}
	MessageBox(0, str, "test", 0);
	while(hm.test++ < 4);
	a = 2;
}