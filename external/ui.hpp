#pragma once
#include "heap.hpp"

namespace Shade
{
	struct UIElement:
		public HeapObject
	{
		int value;
		Ptr<UIElement> next;
	};
	
	Ptr<List<UIElement>> list_ui();
};
