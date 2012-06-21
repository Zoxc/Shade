#include "heap.hpp"

Shade::Heap Shade::heap;

void *Shade::Heap::allocate(size_t bytes)
{
	char *result = current;

	char *next = result + bytes;

	if(next > max)
		return 0;

	current = next;

	return (void *)result;
}

void *Shade::HeapObject::operator new(size_t bytes)
{
	return heap.allocate(bytes);
}

void Shade::HeapObject::operator delete(void *)
{
}