#include "heap.hpp"

namespace Shade
{
	Heap heap;

	void *Heap::allocate(size_t bytes)
	{
		char *result = current;

		char *next = result + bytes;

		if(next > max)
			return 0;

		current = next;

		return (void *)result;
	}

	void Heap::reset()
	{
		current = (char *)start;
	}
	
	void Heap::setup(void *start, size_t size)
	{
		this->start = (size_t)start;
		current = (char *)start;
		max = current + size;
	}
	
	void *HeapObject::operator new(size_t bytes)
	{
		return heap.allocate(bytes);
	}

	void HeapObject::operator delete(void *)
	{
	}
};