#pragma once
#include "external.hpp"

namespace Shade
{
	class D3
	{
	private:
		D3() {}
		
	public:
		struct Module
		{
			size_t delta;
		};
		
		static Module diablo_exe;
		
		template<typename T, size_t offset, Module *module = &diablo_exe> struct Offset
		{
			struct Value
			{
				T ptr;
				
				Value() : ptr((T)(offset + module->delta)) {}
			};
			
			static Value value;
		};
		
		template<class K, class V> struct HashTable
		{
			struct Pair
			{
				Pair *next;
				K key;
				V value;
			};
			
			void *u_0; // Points to &u_3
			void *u_1;
			Pair **table;
			void *u_2[13];
			uint32_t mask; // (table size in power of 2) - 1
			uint32_t entries;
			void *u_3;
			
			template<typename func> bool each_pair(func do_for_pair)
			{
				for(size_t i = 0; i <= mask; ++i)
				{
					Pair *pair = table[i];

					if(pair)
					{
						if(!do_for_pair(pair->key, pair->value))
							return false;
					}
				}
				
				return true;
			}
		};
		
		struct UIReference
		{
			uint64_t hash; 
			char name[0x200];
		} __attribute__((aligned(8)));
		
		struct UIHandler
		{
			const char *name; // Ignored if this is 0
			uint32_t hash;
			
			typedef void (*func_t)(UIReference *element);
			
			func_t execute;
		};
		
		struct UIElement
		{
			/* 1.0.3.10235 Vtable types:
				0x13ED3D8: UIShortcut > UIElement
				0x13ED258: UIDrawHook > UIElement
				0x13D7478: UIEvent > UIElement
				0x13D2BD8: Unknown > UIContainer
				0x13E25B8: UIButton > UIText
				0x13A2760: UILabel > UIText
				0x13D4EB8: UIEdit > UIText
				0x13EAFF0: UICheckbox > UIText
				0x13ECFE0: UIScrollbarThumb > UIContainer
				0x13E8BF0: Unknown > UIContainer
				0x13EB408: Unknown > UIContainer
			*/
			
			void **vtable;
			void *u_0[9];
			uint32_t visible; 
			uint32_t u_1;
			struct UIReference self;
			struct UIReference parent;
		};
		
		struct UIContainer:
			public UIElement
		{
			void *u_0[8];
			struct UIElement **children;
			uint32_t u_1;
			uint32_t child_count;
		};
		
		struct UIText: // size 0xD00?
			public UIContainer
		{
			void *u_0[30];
			uint32_t state;
			void *u_1[25];
			UIHandler::func_t click;
			void *u_2[350];
			const char *text;
			void *u_3[90];
			const char *text_dup;
		};
		
		typedef HashTable<UIReference, UIElement *> UIElementMap;
		typedef HashTable<uint32_t, UIHandler *> UIHandlerMap;

		struct UIManager
		{
			UIElementMap *element_map;
			void *u_0;
			struct UIReference u_1[6];
			void *u_2[1688];
			UIHandlerMap *handler_map;
		};
		
		struct ObjectManager
		{
			void *u_0[585];
			struct UIManager *ui_manager;
		};
		
		enum UIReferenceIndices
		{
			UIReferenceList_None,
			UIReferenceList_Root,
			UIReferenceList_RootNormalLayer,
			UIReferenceList_RootWindowLayer,
			UIReferenceList_RootTopLayer,
			UIReferenceList_MainScreen,
			UIReferenceListEntries,
		};
		
		static constexpr auto &ui_reference_list = Offset<UIReference *, 0x158E3B8>::value.ptr; // 1.0.3.10235
		
		/* ui_handler_list
			1.0.3.10235
			setup_ui_handler_list - 0x1326620: Initializes this array
			setup_ui_manager_handler_list - 0xB52120: populates this list into UIManager::handler_map
			get_ui_handler_from_string - 0xB51F10: maps a string to UIHandler::execute using UIManager::handler_map
		*/
		static constexpr auto &ui_handler_list = Offset<UIHandler *, 0x15924E0>::value.ptr; 
		static const size_t ui_handler_list_size = 920; // referenced in setup_ui_manager_handler_list
		
		static constexpr auto &object_manager = Offset<ObjectManager **, 0x15A1BEC>::value.ptr; // 1.0.3.10235
		
		static constexpr auto &get_UI_element = Offset<UIElement *(__cdecl *)(UIReference *reference), 0x93F400>::value.ptr; // 1.0.3.10235
		
		static void init();
	};
	
	template<typename T, size_t offset, D3::Module *module> typename D3::Offset<T, offset, module>::Value D3::Offset<T, offset, module>::value;
};
