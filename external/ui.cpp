#include "ui.hpp"
#include "call.hpp"

namespace Shade
{
	Ptr<List<UIElement>> list_ui()
	{
		auto list = new List<UIElement>;
		
		auto element = new UIElement;
		
		element->value = 34;
		
		list->append(element);
		
		auto element2 = new UIElement;
		
		element2->value = 76;
		
		list->append(element2);
		
		return list;
	}
};

EXPORT(list_ui, Shade::list_ui, ui_list)