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
		
		struct UIReference
		{
			uint64_t hash; 
			char name[0x200];
		};
		
		struct UIElement
		{
			/* 1.0.3.10235 Vtable types:
				0x13A2760: UILabel, UIText
				0x13D4EB8: UIEdit, UIText
				0x13ED3D8: ?, not UIContainer
				0x13ED258: ?, not UIContainer
				0x13D7478: ?, not UIContainer
			*/
			
			void **vtable;
			void *u_0[9];
			uint32_t visible; 
			uint32_t u_1;
			struct UIReference self;
			struct UIReference parent;
		};
		
		struct UIContainer
		{
			struct UIElement element;
			void *u_4[8];
			struct UIElement **children;
			uint32_t u_5;
			uint32_t child_count;
		};
		
		struct UIText
		{
			struct UIContainer container;
			void *u_6[0x196];
			const char *text;
		};
		
		struct UIHashTablePair
		{
			struct UIHashTablePair *next;
			void *u_0;
			uint64_t key;
			void *u_1[128];
			struct UIElement *value;
		};
		
		struct UIHashTable
		{
			uint32_t u_0[2];
			struct UIHashTablePair **table;
			void *u_1[13];
			uint32_t mask; // (size in power of 2) - 1
			uint32_t entries;
		};
		
		struct UIManager
		{
			struct UIHashTable *hash_table;
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
		
		static constexpr auto &object_manager = Offset<ObjectManager **, 0x15A1BEC>::value.ptr; // 1.0.3.10235
		
		static constexpr auto &get_UI_element = Offset<UIElement *(__cdecl *)(UIReference *reference), 0x93F400>::value.ptr; // 1.0.3.10235
		
		static void init();
	};
	
	template<typename T, size_t offset, D3::Module *module> typename D3::Offset<T, offset, module>::Value D3::Offset<T, offset, module>::value;
};
