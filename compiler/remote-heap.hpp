#pragma once
#include "../shade.hpp"
#include <Prelude/FastList.hpp>

namespace Shade
{
	class RemoteHeap
	{
		struct Page
		{
			Prelude::ListEntry<Page> entry;
			void *address;
			size_t length;
		};

		static const unsigned int max_alloc = 0x1000 * 4;

		private:
			char *current;
			char *max;
			DWORD access;

			Prelude::FastList<Page> pages;

			char *allocate_page(size_t bytes = max_alloc)
			{
				char *result = (char *)VirtualAllocEx(process, 0, bytes, MEM_RESERVE | MEM_COMMIT, access);

				if(!result)
					win32_error("Unable to allocate remote memory");

				Page *page = new Page;

				page->address = result;
				page->length = bytes;

				pages.append(page);

				return result;
			}

			void *get_page(size_t bytes, size_t alignment)
			{
				if(prelude_unlikely(bytes > max_alloc))
					return allocate_page(bytes);

				char *result = allocate_page();
				
				max = result + max_alloc;
				
				result = (char *)Prelude::align((size_t)result, alignment);

				current = result + bytes;
		
				return result;
			}

		public:
			RemoteHeap(DWORD access) :
				access(access),
				current(0),
				max(0)
			{
			}

			~RemoteHeap()
			{
				auto page = pages.begin();

				while(page != pages.end())
				{
					Page *current = *page;
					++page;
					
					//VirtualFreeEx(process, current->address, 0, MEM_RELEASE);

					delete current;
				};
			}
			
			void *allocate(size_t bytes, size_t alignment)
			{
				char *result = (char *)Prelude::align((size_t)current, alignment);

				char *next = result + bytes;
		
				if(prelude_unlikely(next > max))
					return get_page(bytes, alignment);

				current = next;

				return (void *)result;
			}
	};
};
