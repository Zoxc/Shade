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
			ListUI,
			ListUIHandlers,
			Dummy
		};
	};

	namespace Error
	{
		enum Type
		{
			None,
			Unknown,
			OutOfMemory,
			NotFound
		};
	};

	struct Shared
	{
		static const size_t mapping_size = 0x2000000;
		
		volatile Call::Type call_type;
		volatile Error::Type error_type;
		HANDLE event_start;
		
		HANDLE event_end;
		HANDLE event_thread; // Must follow event_end
		
		size_t d3d_present_offset;
		void *d3d_present;
		bool triggered;
		struct {
			Ptr<Remote::UIElement> ui_root;
			Ptr<List<Remote::UIHandler>> ui_handlers;
			size_t num;
			void *ptr;
		} result;
	};
	
	extern Shared *shared;
};
