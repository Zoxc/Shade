#pragma once
#include "shade.hpp"
#include "external/call.hpp"

namespace Shade
{
	struct Remote
	{
		void *call;
		void *list_ui;
		HANDLE event_handle;
		HANDLE memory;
	};
	
	struct Local
	{
		HANDLE event_handle;
		HANDLE memory;
	};
	
	extern Remote remote;
	extern Local local;
	
	void allocate_shared_memory();
	
	double avg_time_per_remote_call();

	void remote_event(void *ip, bool paused = false);

	void create_process();
};
