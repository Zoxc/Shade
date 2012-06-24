#include "ui.hpp"
#include "shared.hpp"

namespace Shade
{
	namespace Remote
	{
		Ptr<List<UIElement>> list_ui()
		{
			return 0;
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
};
