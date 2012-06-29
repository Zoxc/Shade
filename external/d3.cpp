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

#define verify_offset(struct, field, offset) OffsetTest<Shade::D3::struct, decltype(Shade::D3::struct::field), &Shade::D3::struct::field, __builtin_offsetof(Shade::D3::struct, field), offset>::test()

void verify_offsets()
{
	verify_offset(ObjectManager, ui_manager, 0x924);

	verify_offset(UIHashTable, table, 0x8);
	verify_offset(UIHashTable, mask, 0x40);

	verify_offset(UIReference, name, 0x8);
	
	verify_offset(UIElement, self, 0x30);
	
	// Referenced by 0x941360 - 1.0.3.10235
	verify_offset(UIElement, parent, 0x238);
	
	// UIElement::IterateList - 0A8AA00 - 1.0.3.10235
	// UIElement::ConditionalIteration - 00A897C0 - 1.0.3.10235
	verify_offset(UIContainer, children, 0x460);
	verify_offset(UIContainer, child_count, 0x468);
	
	verify_offset(UIText, text, 0xAC8);
	
	verify_offset(UIHashTablePair, value, 0x210);
}
