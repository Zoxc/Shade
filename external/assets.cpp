#include "ui.hpp"
#include "shared.hpp"
#include "d3.hpp"

namespace Shade
{
	namespace Remote
	{
		void list_actor_assets()
		{
			auto actors = new List<Actor>;
			
			auto list = (*D3::game_data)->get_object_list("RActors");
			
			list->each_object<D3::Actor>([&](D3::Actor *d3_actor) {
				auto actor = new Actor;
				
				actor->ptr = d3_actor;
				actor->id = d3_actor->id;
				actor->acd_id = d3_actor->common_data_id;
				actor->name = new String(d3_actor->name, sizeof(D3::Actor::name));
				
				actors->append(actor);
			});
			
			shared->data.actors = actors;
		}
		
		void list_acd_assets()
		{
			auto acds = new List<ActorCommonData>;
			
			auto list = (*D3::game_data)->get_object_list("ActorCommonData");
			
			list->each_object<D3::ActorCommonData>([&](D3::ActorCommonData *d3_acd) {
				auto acd = new ActorCommonData;
				
				acd->ptr = d3_acd;
				acd->id = d3_acd->id;
				acd->owner_id = d3_acd->owner_id;
				acd->name = new String(d3_acd->name, sizeof(D3::ActorCommonData::name));
				
				acds->append(acd);
			});
			
			shared->data.acds = acds;
		}
	};
};
