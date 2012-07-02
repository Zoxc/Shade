#pragma once
#include "utils.hpp"

namespace Shade
{
	namespace Remote
	{
		struct UIElement:
			public HeapObject
		{
			void *vtable;
			void *ptr;
			uint64_t hash;
			bool visible;
			bool skipped_children;
			Ptr<String> name;
			Ptr<String> text;
			
			Vector<Ptr<UIElement>> children;
		};
		
		struct UIHandler:
			public HeapObject
		{
			Ptr<UIHandler> next;
			
			Ptr<String> name;
			void *func;
			uint32_t hash;
		};
		
		void list_ui();
		void list_ui_handlers();
	};
};
