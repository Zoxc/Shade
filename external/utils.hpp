#pragma once
#include "heap.hpp"

namespace Shade
{
	template<class T> struct Ptr
	{
		size_t offset;
		
		T *operator =(T *ptr)
		{
			set(ptr);
			return ptr;
		}
		
		T *operator ->()
		{
			return get();
		}
		
		operator bool()
		{
			return offset != (size_t)-1;
		}
		
		operator T *()
		{
			return get();
		}
		
		Ptr() : offset(-1)
		{
		}
		
		Ptr(T *ptr)
		{
			set(ptr);
		}
		
		void set(T *ptr)
		{
			offset = ptr == nullptr ? (size_t)-1 : (size_t)ptr - heap.start;
		}
		
		T *get()
		{
			if(offset != (size_t)-1)
				return (T *)(offset + heap.start);
			else
				return nullptr;
		}
	};
	
	template<class T> struct Vector:
		public HeapObject
	{
		size_t size;
		Ptr<T> table;
		
		T &operator [](size_t index)
		{
			return table.get()[index];
		}
		
		void allocate(size_t size)
		{
			this->size = size;
			table = (T *)heap.allocate(size * sizeof(T));
		}
		
		T *begin()
		{
			return table.get();
		}
		
		T *end()
		{
			return table.get() + size;
		}
		
		Vector() : size(0)
		{
		}
		
		Vector(size_t size)
		{
			allocate(size);
		}
	};
	
	template<class T> class ListObject:
		public HeapObject
	{
	public:
		Ptr<T> next;
	};
	
	template<class T, Ptr<T> T::*field = &T::next> class List:
		public HeapObject
	{
	public:
		Ptr<T> first;
		Ptr<T> last;

		bool empty()
		{
			return !first;
		}

		void append(T *node)
		{
			node->*field = (T *)0;
			
			if(last)
			{
				last->*field = node;
				last = node;
			}
			else
			{
				first = node;
				last = node;
			}
		}

		class Iterator
		{
		private:
			T *current;

		public:
			Iterator(T *start) : current(start) {}

			void step()
			{
				current = current->*field;
			}
			
			bool operator ==(const Iterator &other) const
			{
				return current == other.current;
			}
			
			bool operator !=(const Iterator &other) const
			{
				return current != other.current;
			}
			
			T &operator ++()
			{
				step();
				return *current;
			}
			
			T &operator ++(int)
			{
				T *result = current;
				step();
				return *result;
			}
			
			T *operator*() const
			{
				return current;
			}

			T &operator ()() const
			{
				return *current;
			}
		};
		
		Iterator begin()
		{
			return Iterator(first);
		}

		Iterator end()
		{
			return Iterator(0);
		}
	};
	
	class String:
		public HeapObject
	{
	private:
		size_t size;
		Ptr<const char> data;
		
		void setup(const char *str, size_t length);
		
	public:
		const char *c_str();
		
		String(const char *str);
		String(const char *str, size_t length);
	};
};