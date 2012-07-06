#pragma once
#include "utils.hpp"

namespace Shade
{
	namespace Remote
	{
		struct ActorCommonData:
			public HeapObject
		{
			void *ptr;
			Ptr<String> name;
			size_t id;
			size_t actor_id;
			
			Ptr<ActorCommonData> next;
		};
		
		struct Actor:
			public HeapObject
		{
			void *ptr;
			Ptr<String> name;
			size_t id;
			size_t acd_id;
			
			Ptr<Actor> next;
		};
		
		void list_actor_assets();
		void list_acd_assets();
	};
};
