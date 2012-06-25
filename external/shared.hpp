#pragma once
#include "heap.hpp"
#include "ui.hpp"

namespace Shade
{
	namespace Call
	{
		enum Type
		{
			Continue,
			Dummy
		};
	};

	struct Shared
	{
		Call::Type call_type;
		HANDLE event_start;
		
		HANDLE event_end;
		HANDLE event_thread; // Must follow event_end
		
		size_t d3d_present_offset;
		void *d3d_present;
		volatile bool triggered;
		struct {
			Ptr<List<Remote::UIElement>> ui_list;
			size_t num;
			void *ptr;
		} result;
	};
	
	extern Shared *shared;
};
