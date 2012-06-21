#pragma once
#include "shade.hpp"

namespace Shade
{
	extern HANDLE local_event_handle;
	extern HANDLE remote_event_handle;

	void remote_event(void *ip, bool paused = false);

	void create_process();
};
