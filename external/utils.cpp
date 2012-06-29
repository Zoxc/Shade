#include "utils.hpp"

namespace Shade
{
	void String::setup(const char *str, size_t length)
	{
		size = length;
		
		auto data_ptr = (char *)heap.allocate(size + 1);
		memcpy(data_ptr, str, size);
		data_ptr[size] = 0;
		
		data = data_ptr;
	}
	
	const char *String::c_str()
	{
		return data;
	}
	
	String::String(const char *str)
	{
		setup(str, strlen(str));
	}
	
	String::String(const char *str, size_t length)
	{
		setup(str, strnlen(str, length));
	}
};