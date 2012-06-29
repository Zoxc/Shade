#include "ui.hpp"
#include "shared.hpp"
#include "d3.hpp"

namespace Shade
{
	namespace Remote
	{
		Ptr<String> get_text(D3::UIElement *element)
		{
			bool text;
			
			switch((size_t)element->vtable)
			{
				case 0x13A2760:
				case 0x13D4EB8:
					text = true;
					break;
					
				default:
					text = false;
			}
			
			if(text)
			{
				auto text_str = ((D3::UIText *)element)->text;
				
				if(text_str)
				{
					shared->result.num = *text_str;
					return new String(text_str);
				}
			}
			
			return nullptr;
		}
		
		UIElement *copy_element(D3::UIElement *d3_element);
		
		void copy_element_children(UIElement *element, D3::UIElement *d3_element)
		{
			switch((size_t)d3_element->vtable)
			{
				case 0x13ED3D8:
				case 0x13ED258:
				case 0x13D7478:
					element->skipped_children = true;
					break;
					
				default:
					element->skipped_children = false;
			}
			
			if(!element->skipped_children)
			{
				auto container = (D3::UIContainer *)d3_element;
				
				element->children.allocate(container->child_count);
				
				for(size_t i = 0; i < container->child_count; ++i)
					element->children[i] = copy_element(container->children[i]);
			}
		}
		
		UIElement *copy_element(D3::UIElement *d3_element)
		{
			auto element = new UIElement;
			
			element->ptr = d3_element;
			element->name = new String(d3_element->self.name, sizeof(D3::UIReference::name));
			
			element->text = get_text(d3_element);
			
			element->visible = d3_element->visible != 0;
			element->hash = d3_element->self.hash;
			
			element->vtable = d3_element->vtable;
			
			copy_element_children(element, d3_element);
			
			return element;
		}
		
		void list_ui()
		{
			auto d3_root = D3::get_UI_element(&D3::ui_reference_list[D3::UIReferenceList_Root]);
			
			shared->result.ui_root = copy_element(d3_root);
		}
	};
};
