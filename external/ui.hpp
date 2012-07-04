#pragma once
#include "utils.hpp"

namespace Shade
{
	namespace Remote
	{
		struct UIRect:
			public HeapObject
		{
			float left;
			float top;
			float right;
			float bottom;
		};
		
		struct UIElement:
			public HeapObject
		{
			void *v_table;
			void *ptr;
			uint64_t hash;
			bool visible;
			Ptr<String> name;
			Ptr<String> text;
			Ptr<UIRect> rect;
			
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
