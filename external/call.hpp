#pragma once
#include "heap.hpp"
#include "ui.hpp"

typedef unsigned int size_t;

namespace Shade
{
	class Call
	{
	public:
		HANDLE event;
		HANDLE memory;
		
		struct {
			Ptr<List<UIElement>> ui_list;
			void *ptr;
		} result;
		
		void done();
	};
	
	extern Call call;
};

#define EXPORT(name, impl, resultvar) extern "C" void __cdecl name(void) { Shade::heap.reset(); Shade::call.result.resultvar = impl(); Shade::call.done(); }
