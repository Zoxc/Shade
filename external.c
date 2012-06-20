#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

int a = 1;
int b;

void init()
{
	MessageBox(0, "Hello", "test", 0);
	while(a++ < 4);
	a = 2;
}