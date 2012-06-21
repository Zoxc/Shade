#pragma once
#include "shade.hpp"

namespace Shade
{
	extern HANDLE local_event_handle;
	extern HANDLE remote_event_handle;

	template<typename F> void remote_event(F func, bool paused = false)
	{
		if(!paused)
		{
			if(SuspendThread(thread) == (DWORD)-1)
				win32_error("Unable to pause remote thread");
		}

		CONTEXT context;

		context.ContextFlags = CONTEXT_ALL;

		if(!GetThreadContext(thread, &context))
			win32_error("Unable to get remote thread context");
	
		CONTEXT new_context = context;
	
		new_context.Esp -= 32; // Skip some bytes on the stack
		new_context.Esp = new_context.Esp & ~15; // Align to 16 byte boundary

		func(new_context);

		if(!SetThreadContext(thread, &new_context))
			win32_error("Unable to set remote thread context");
	
		if(ResumeThread(thread) == (DWORD)-1)
			win32_error("Unable to start remote code");

		if(WaitForSingleObject(local_event_handle, INFINITE) == WAIT_FAILED)
			win32_error("Unable to wait for remote code");
		
		if(SuspendThread(thread) == (DWORD)-1)
			win32_error("Unable to pause remote thread");
		
		if(!SetThreadContext(thread, &context))
			win32_error("Unable to restore remote thread context");
	
		if(ResumeThread(thread) == (DWORD)-1)
			win32_error("Unable to resume remote thread");
	}

	void create_process();
};
