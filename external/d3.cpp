#include "d3.hpp"

namespace Shade
{
	D3::Module D3::diablo_exe;
	
	void D3::init()
	{
		diablo_exe.delta = (size_t)GetModuleHandle(0) - 0x800000;
	}
};

template<class Struct, class FieldType, FieldType Struct::*field, size_t found_offset, size_t expected_offset> struct OffsetTest
{
	static_assert(found_offset == expected_offset, "Invalid offset");
	
	static void test() {}
};

template<class Struct, size_t found_size, size_t expected_size> struct SizeTest
{
	static_assert(found_size == expected_size, "Invalid size");
	
	static void test() {}
};

#define verify_offset(struct, field, offset) OffsetTest<Shade::D3::struct, decltype(Shade::D3::struct::field), &Shade::D3::struct::field, __builtin_offsetof(Shade::D3::struct, field), offset>::test()

#define verify_size(struct, size) SizeTest<Shade::D3::struct, sizeof(Shade::D3::struct), size>::test()

void verify_offsets()
{
	verify_offset(ActorCommonData, guid_0, 0x88);
	verify_offset(ActorCommonData, acd_gball, 0xB4);
	verify_offset(ActorCommonData, position, 0xD0);
	verify_offset(ActorCommonData, default_radius, 0x100);
	verify_offset(ActorCommonData, world, 0x108);
	verify_offset(ActorCommonData, owner_id, 0x110);
	verify_offset(ActorCommonData, attributes, 0x120);
	verify_offset(ActorCommonData, radius_type, 0x21D);
	verify_offset(ActorCommonData, radius_type, 0x21D);
	verify_offset(ActorCommonData, scaled_radius, 0x238);
	
	// Referenced by AssetList::asset_size
	verify_size(ActorCommonData, 0x2D0);
	
	verify_offset(ActorMovement, current_speed, 0xC);
	verify_offset(ActorMovement, scale, 0x1C);
	verify_offset(ActorMovement, flags, 0x20);
	verify_offset(ActorMovement, moveable, 0x34);
	verify_offset(ActorMovement, walkable, 0x38);
	verify_offset(ActorMovement, moving_to, 0x3C);
	verify_offset(ActorMovement, position_0, 0x4C);
	verify_offset(ActorMovement, tp, 0x74);
	verify_offset(ActorMovement, position_1, 0xA4);
	verify_offset(ActorMovement, speed_1, 0xB8);
	verify_offset(ActorMovement, direction, 0x170);
	
	verify_offset(Actor, sno_id, 0x88);
	verify_offset(Actor, default_radius, 0xD0);
	verify_offset(Actor, position_3, 0x100);
	verify_offset(Actor, fag_id, 0x120);
	verify_offset(Actor, position_4, 0x140);
	verify_offset(Actor, u_7, 0x15C);
	verify_offset(Actor, position_5, 0x210);
	verify_offset(Actor, movement, 0x380);
	verify_offset(Actor, velocity, 0x3A0);
	verify_offset(Actor, frame, 0x418);
	
	// Referenced by AssetList::asset_size
	verify_size(Actor, 0x428);
	
	verify_offset(AttributeMap, table, 0x8);
	verify_offset(AttributeMap, mask, 0x418);

	// Referenced by AssetList::asset_size
	verify_size(AttributeAsset, 0x180);
	
	verify_offset(AttributeAsset, attribute_map, 0x10);
	
	verify_offset(AssetList, asset_size, 0x104);
	verify_offset(AssetList, asset_count, 0x108);
	verify_offset(AssetList, assets, 0x148);
	
	verify_offset(GameData, asset_lists, 0x390);
	
	verify_offset(ObjectManager, count_0, 0x1C);
	verify_offset(ObjectManager, ui_manager, 0x924);
	
	verify_offset(UIManager, component_map, 0);
	verify_offset(UIManager, handler_map, 0x2698);
	
	// Referenced by dispatch_ui_event
	verify_size(UIEvent, 0x48);
	
	verify_size(UIReference, 0x208);
	verify_offset(UIReference, name, 0x8);
	
	verify_offset(UIComponentVirtualTable, event, 0x1C);
	verify_offset(UIComponentVirtualTable, mouse_move, 0x30);
	verify_offset(UIComponentVirtualTable, mouse_enter, 0x44);
	verify_offset(UIComponentVirtualTable, mouse_leave, 0x48);
	verify_offset(UIComponentVirtualTable, set_text, 0x84);
	verify_offset(UIComponentVirtualTable, switch_state, 0x90);
	
	verify_offset(UIComponent, self, 0x30);
	
	// Referenced by 0x941360 - 1.0.3.10235
	verify_offset(UIComponent, parent, 0x238);
	
	// Referenced by 0xAE27A0 - 1.0.3.10235
	verify_offset(UIComponent, handler_0, 0x8);
	verify_offset(UIComponent, handler_1, 0x10);
	verify_offset(UIComponent, sound_0, 0x1C);
	
	// UIContainer::PassEventToChildren - 0A8AA00 - 1.0.3.10235
	// UIContainer::IterateVisible - 00A897C0 - 1.0.3.10235
	verify_offset(UIContainer, children, 0x460);
	verify_offset(UIContainer, child_count, 0x468);
	
	// Referenced by 0x9D5820 - 1.0.3.10235
	verify_offset(UIControl, text, 0xAC8);
	
	verify_offset(UIControl, text_dup, 0xC34);
	verify_offset(UIControl, state, 0x4E4);
	
	// Referenced by 0xBA75C0 (event handler) - 1.0.3.10235
	verify_offset(UIControl, click, 0x54C);
	
	// Referenced by extract_ui_rect
	verify_offset(UIControl, rect_0, 0x508);
	verify_offset(UIControl, rect_1, 0xA5C);
	verify_offset(UIControl, rect_2, 0xA9C);
	
	verify_offset(UIComponentMap, table, 0x8);
	verify_offset(UIComponentMap, mask, 0x40);

	verify_offset(UIComponentMap::Pair, key, 0x8);
	verify_offset(UIComponentMap::Pair, value, 0x210);
}
