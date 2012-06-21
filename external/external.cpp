#include "external.hpp"
#include "call.hpp"

void *Shade::init()
{
	size_t mapping_size = 0x1000;
	void *mapping = MapViewOfFile(call.memory, FILE_MAP_ALL_ACCESS, 0, 0, mapping_size);

	if(!mapping)
	{
		MessageBox(0, "Failed to allocate remote view!", "Shade", 0);
		return (void *)1;
	}
	
	heap.setup(mapping, mapping_size);
	
	return 0;
}

EXPORT(init, Shade::init, ptr)