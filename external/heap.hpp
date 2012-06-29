#pragma once
#include "external.hpp"

namespace Shade
{
	class Heap
	{
		char *current;
		char *max;

	public:
		size_t start;
	
		void *allocate(size_t bytes);
		void setup(void *start, size_t size);
		void reset();
	};

	class HeapObject
	{
	public:
		static void *operator new(size_t bytes);
		static void operator delete(void *);
	};
	
	extern Heap heap;
};