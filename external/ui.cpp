#include "ui.hpp"
#include "shared.hpp"
#include "d3.hpp"

namespace Shade
{
	namespace Remote
	{
		void do_control(UIElement *element, D3::UIComponent *component)
		{
			switch((size_t)component->v_table)
			{
				case 0x13E25B8:
				case 0x13A2760:
				case 0x13D4EB8:
					break;
					
				default:
					return;
			}
			
			auto control = (D3::UIControl *)component;
			
			if(control->text)
				element->text = new String(control->text);
			
			auto rect = new UIRect;
			
			D3::UIRect d3_rect;
			
			D3::extract_ui_rect(control, &d3_rect);
			D3::map_ui_rect(&d3_rect, &d3_rect, true, true);
			
			rect->left = d3_rect.left;
			rect->top = d3_rect.top;
			rect->right = d3_rect.right;
			rect->bottom = d3_rect.bottom;
			
			element->rect = rect;
		}
		
		UIElement *copy_element(D3::UIComponent *component);
		
		void do_container(UIElement *element, D3::UIComponent *component)
		{
			switch((size_t)component->v_table)
			{
				case 0x13ED3D8:
				case 0x13ED258:
				case 0x13D7478:
					return;
					
				default:
					break;
			}
		
			auto container = (D3::UIContainer *)component;
			
			element->children.allocate(container->child_count);
			
			for(size_t i = 0; i < container->child_count; ++i)
				element->children[i] = copy_element(container->children[i]);
		}
		
		UIElement *copy_element(D3::UIComponent *component)
		{
			auto element = new UIElement;
			
			element->ptr = component;
			element->name = new String(component->self.name, sizeof(D3::UIReference::name));
			
			element->visible = component->visible != 0;
			element->hash = component->self.hash;
			
			element->v_table = component->v_table;
			
			do_control(element, component);
			do_container(element, component);
			
			return element;
		}
		
		void list_ui()
		{
			auto d3_root = D3::get_ui_component(&D3::ui_reference_list[D3::UIReferenceList_Root]);
			
			shared->result.ui_root = copy_element(d3_root);
		}
		
		void list_ui_handlers()
		{
			auto handlers = new List<UIHandler>;
			
			for(size_t i = 0; i < D3::ui_handler_list_size; ++i)
			{
				auto d3_handler = &D3::ui_handler_list[i];
				
				if(!d3_handler->name)
					continue;
				
				auto handler = new UIHandler;
				
				handler->func = d3_handler->execute;
				handler->hash = d3_handler->hash;
				handler->name = new String(d3_handler->name);
				
				handlers->append(handler);
			}
			
			shared->result.ui_handlers = handlers;
		}
	};
};
