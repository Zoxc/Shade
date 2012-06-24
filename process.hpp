#pragma once
#include "shade.hpp"

namespace Shade
{
	struct Event
	{
		HANDLE event;
		HANDLE thread;
	};
	
	struct Local
	{
		Event start;
		Event end;
		HANDLE memory;
	};
	
	extern HANDLE remote_memory;
	extern Local local;
	
	void allocate_shared_memory();
	
	void remote_event(void *ip, bool paused);

	double avg_time_per_remote_call();

	void create_event(Event &local, HANDLE &remote);
	void wait_event(Event &event);
	void reset_event(Event &event);
	void signal_event(Event &event);
	
	void create_process();
	void resume_process();
};
