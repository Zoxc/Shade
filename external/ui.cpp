#include "ui.hpp"
#include "call.hpp"

namespace Shade
{
	Ptr<List<UIElement>> list_ui()
	{
		auto list = new List<UIElement>;
		
		auto element = new UIElement;
		
		list->append(element);
		
		return list;
	}
};

EXPORT(list_ui, Shade::list_ui, ui_list)