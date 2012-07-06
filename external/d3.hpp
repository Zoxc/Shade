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
		
		template<class T> struct LinkedList
		{
			struct Node
			{
				T value;
				Node *prev;
				Node *next;
			};
			
			Node *first;
			Node *last;
			size_t count;
		};
		
		template<class K, class V, size_t inline_slots = 10> struct HashTable
		{
			struct Pair
			{
				Pair *next;
				K key;
				V value;
			};
			
			void *u_0; // Points to &HashTable + 1 in UI hash tables. User-data field?
			void *u_1;
			Pair **table;
			void *u_2;
			uint32_t table_size;
			Pair *inline_table[inline_slots];
			void *u_3;
			uint32_t mask; // (table size in power of 2) - 1
			uint32_t entries;
			
			template<typename func> bool each_pair(func do_for_pair)
			{
				for(size_t i = 0; i <= mask; ++i)
				{
					Pair *pair = table[i];

					while(pair)
					{
						if(!do_for_pair(pair->key, pair->value))
							return false;
							
						pair = pair->next;
					}
				}
				
				return true;
			}
		};
		
		/* UIEvent
			1.0.3.10235
			MainWndProc - 0x80E5F0: Setups this structure
			dispatch_ui_event - 0x94A8E0: Dispatches the event
		*/
		struct UIEvent
		{
			enum Type
			{
				LeftMouseDown = 6,
				LeftMouseUp = 7,
				LeftMouseDblClick = 8,
				
				MiddleMouseDown = 11,
				MiddleMouseUp = 12,
				MiddleMouseDblClick = 13,
				
				RightMouseDown = 16,
				RightMouseUp = 17,
				RightMouseDblClick = 18,
				
			};
			
			Type type;
			
			void *u_0[6];
			int x;
			int y;
			void *u_1[9];
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
		
		struct UIComponent;
		
		struct UIComponentVirtualTable
		{
			void *u_0[7];
			void (__thiscall *event)(UIComponent *self, int *, UIEvent *event);
			void *u_1[4];
			void (__thiscall *mouse_move)(UIComponent *self);
			void *u_2[4];
			void (__thiscall *mouse_enter)(UIComponent *self);
			void (__thiscall *mouse_leave)(UIComponent *self);
			void *u_3[14];
			void (__thiscall *set_text)(UIComponent *self, const char *text, int);
			void *u_4[2];
			void (__thiscall *switch_state)(UIComponent *self, int, int);
		};
		
		typedef int sound_t; // -1 is no sound
		typedef int guid_t; // -1 is no GUID
		
		struct UIRect
		{
			float left;
			float top;
			float right;
			float bottom;
		};
		
		struct UIComponent
		{
			/* 1.0.3.10235 Vtable types:
				0x13ED3D8: UIShortcut > UIComponent
				0x13ED258: UIDrawHook > UIComponent
				0x13D7478: UIEvent > UIComponent
				0x13E3D80: UITimer > UIComponent
				0x13D2BD8: Unknown > UIContainer
				0x13E25B8: UIButton > UIControl
				0x13A2760: UILabel > UIControl
				0x13D4EB8: UIEdit > UIControl
				0x13EAFF0: UICheckbox > UIControl
				0x13ECFE0: UIScrollbarThumb > UIContainer
				0x13E8BF0: Unknown > UIContainer
				0x13EB408: Unknown > UIContainer
			*/
			
			UIComponentVirtualTable *v_table;
			void *u_0;
			UIHandler::func_t handler_0;
			void *u_1[1];
			UIHandler::func_t handler_1;
			void *u_2[2];
			sound_t sound_0;
			void *u_3[2];
			uint32_t visible; 
			uint32_t u_4; // Padding?
			UIReference self;
			UIReference parent;
		};
		
		struct UIContainer:
			public UIComponent
		{
			void *u_0[8];
			UIComponent **children;
			uint32_t u_1;
			uint32_t child_count;
		};
		
		struct UIControl: // size 0xD00?
			public UIContainer
		{
			void *u_0[30];
			uint32_t state;
			void *u_1[1];
			uint32_t flags; // Controls which UIRect is used
			void *u_2[6];
			UIRect rect_0;
			void *u_3[13];
			UIHandler::func_t click;
			void *u_4[323];
			UIRect rect_1;
			void *u_5[12];
			UIRect rect_2;
			void *u_6[7];
			const char *text;
			void *u_7[90];
			const char *text_dup;
		};
		
		typedef HashTable<UIReference, UIComponent *> UIComponentMap;
		typedef HashTable<uint32_t, UIHandler *> UIHandlerMap;

		struct UIManager
		{
			UIComponentMap *component_map;
			void *u_0; // Padding?
			UIReference u_1[6];
			void *u_2[1688];
			UIHandlerMap *handler_map;
		};
		
		struct ObjectManager
		{
			void *u_0[7];
			int count_0; // Fires UIComponent::handler_1 count_0 times in 0xAE27A0 - 1.0.3.10235
			void *u_1[577];
			UIManager *ui_manager;
		};
		
		template<size_t size> struct Vector
		{
			float array[size];
		};
		
		struct ActorCommonData
		{
			guid_t self_id;
			char name[0x80];
			void *u_84;
			guid_t guid_0;
			guid_t guid_1;
			guid_t guid_2;
			void *u_94[8];
			guid_t acd_gball;
			void *u_B8[6];
			Vector<3> position;
			void *u_DC[9];
			float default_radius;
			void *u_104;
			guid_t world;
			void *u_10C;
			guid_t owner_id;
			uint32_t item_location;
			uint32_t item_x;
			uint32_t item_y;
			uint32_t attributes;
			void *u_124[62];
			char u_21C;
			char radius_type;
			char u_21E;
			char u_21F;
			void *u_220[6];
			float scaled_radius;
			void *u_23C[37];
		};
		
		struct ActorMovement
		{
			void **v_table;
			uint32_t active;
			float speed_0;
			float current_speed;
			void *u_C[3];
			float scale;
			uint32_t flags;
			void *u_24[4];
			uint32_t moveable;
			uint32_t walkable;
			Vector<3> moving_to;
			void *u_48;
			Vector<3> position_0;
			void *u_58[7];
			Vector<3> tp;
			void *u_80[9];
			Vector<3> position_1;
			void *u_B0[2];
			float speed_1;
			void *u_BC[40];
			guid_t actor_id;
			uint32_t frame;
			uint32_t frame_movement;
			uint32_t prev_frame;
			void *u_16C;
			float direction;
		};
		
		struct Actor
		{
			guid_t self_id;
			guid_t common_data_id;
			char name[0x80];
			guid_t sno_id;
			void *u_0;
			Vector<4> rotation;
			Vector<3> position_0;
			float float_0;
			Vector<3> position_1;
			float float_1;
			Vector<3> position_2;
			void *u_1;
			float default_radius;
			void *u_2;
			guid_t world_id;
			guid_t guid_0;
			void *u_3[8];
			Vector<3> position_3;
			void *u_4[5];
			guid_t fag_id;
			void *u_5[7];
			Vector<3> position_4;
			void *u_6[4];
			uint32_t u_7;
			void *u_8[44];
			Vector<3> position_5;
			void *u_9[89];
			ActorMovement *movement;
			float direction;
			void *u_10[6];
			Vector<3> velocity;
			Vector<3> position_6;
			void *u_11[22];
			uint64_t alive;
			uint32_t frame;
			uint32_t difference;
			void *u_12[2];
		};
		
		typedef HashTable<uint32_t, uint32_t, 0x100> AttributeMap;
		
		struct AttributeAsset
		{
			guid_t guid_0;
			uint32_t u_0[3];
			AttributeMap *attribute_map;
			void *u_1; // (size_t)attribute_map + 0x428 - Probably a subfield pointer
			guid_t guid_1;
			void *u_2[89];
		};
		
		struct AssetList
		{
			char type[252];
			void *u_0[2];
			size_t asset_size;
			size_t asset_count;
			uint32_t u_1[2];
			LinkedList<uint32_t> list_0;
			void *u_2; // Points to &u_3
			void *u_3[9];
			void **assets;
			
			template<typename Asset, typename F> void each_asset(F func)
			{
				size_t current = (size_t)*assets;
				
				for(size_t i = 0; i < asset_count; ++i)
				{
					func((Asset *)current);
					
					current += asset_size;
				}
			}
		};
		
		struct GameData
		{
			void *u_0[228];
			LinkedList<AssetList *> asset_lists;
			
			AssetList *get_asset_list(const char *type)
			{
				auto node = asset_lists.first;
				
				while(node)
				{
					if(strncmp(type, node->value->type, sizeof(AssetList::type)) == 0)
						return node->value;
					
					node = node->next;
				}
				
				return 0;
			}
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
		
		/* window_not_minimized
			This variable is set to 0 when the window is minimized and to 1 when it isn't.
			1.0.3.10235
			MainWndProc - 0x80E5F0: Referenced in a function called at the end
		*/
		static constexpr auto &window_not_minimized = Offset<uint32_t **, 0x157D2AC>::value.ptr;
		
		/* game_data
			1.0.3.10235
			Referenced in start of 0x9A62E0
		*/
		static constexpr auto &game_data = Offset<GameData **, 0x15A2EA4>::value.ptr;
		
		static constexpr auto &get_ui_component = Offset<UIComponent *(__cdecl *)(UIReference *reference), 0x93F400>::value.ptr; // 1.0.3.10235
		
		/* extract_ui_rect
			Extracts the virtual coordinates of the UIControl into the rect parameter
		*/
		static constexpr auto &extract_ui_rect = Offset<void (__thiscall *)(UIControl *element, UIRect *rect), 0xA85B80>::value.ptr; // 1.0.3.10235
		
		/* map_ui_rect
			Maps the virtual coordinates passed in 'in' and maps it to window coordinates which is stored in 'out'
		*/
		static constexpr auto &map_ui_rect = Offset<void (*)(UIRect *in, UIRect *out, bool x_axis, bool y_axis), 0xA8A230>::value.ptr; // 1.0.3.10235
		
		static void init();
	};
	
	template<typename T, size_t offset, D3::Module *module> typename D3::Offset<T, offset, module>::Value D3::Offset<T, offset, module>::value;
};
