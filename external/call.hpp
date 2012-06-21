#pragma once
#include "heap.hpp"
#include "ui.hpp"

typedef unsigned int size_t;

namespace Shade
{
	struct Info
	{
		CONTEXT context;
		struct {
			Ptr<List<UIElement>> ui_list;
			void *ptr;
		} result;
	};
	
	class Call
	{
	public:
		HANDLE event;
		HANDLE memory;
		
		void setup();
		__declspec(noreturn) void done();
	};
	
	extern Info *info;
	extern Call call;
};

#define EXPORT(name, impl, resultvar) extern "C" void __cdecl name(void) { Shade::call.setup(); Shade::info->result.resultvar = impl(); Shade::call.done(); }
