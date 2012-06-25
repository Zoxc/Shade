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
		
		static constexpr auto &iterate_UI = Offset<int(__thiscall *)(void *, int, int), 0xA89200>::value.ptr;
		
		static void init();
	};
	
	template<typename T, size_t offset, D3::Module *module> typename D3::Offset<T, offset, module>::Value D3::Offset<T, offset, module>::value;
};
