#include "external.hpp"
#include "call.hpp"

void *Shade::init()
{
	MessageBox(0, "Welcome!", "Shade", 0);
	
	return 0;
}

EXPORT(init, Shade::init, ptr)