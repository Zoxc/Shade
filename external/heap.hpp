#pragma once
#include "external.hpp"

namespace Shade
{
	class Heap
	{
		char *current;
		char *max;

	public:
		size_t start;
	
		void *allocate(size_t bytes);
		void setup(void *start, size_t size);
		void reset();
	};

	class HeapObject
	{
	public:
		static void *operator new(size_t bytes);
		static void operator delete(void *);
	};

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
			return first.get() == 0;
		}

		void append(T *node)
		{
			node->*field = 0;
			
			if(last != 0)
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
	
	extern Heap heap;
};