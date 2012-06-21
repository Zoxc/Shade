#include "process.hpp"
#include <shellapi.h>

HANDLE Shade::thread;
HANDLE Shade::process;
HANDLE Shade::local_event_handle;
HANDLE Shade::remote_event_handle;

void Shade::remote_event(void *ip, bool paused)
{
	if(!ResetEvent(local_event_handle))
			win32_error("Unable to reset event");

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
	
	new_context.Eip = (DWORD)ip;
	new_context.Esp -= 128; // Skip some bytes on the stack
	new_context.Esp = new_context.Esp & ~15; // Align to 16 byte boundary
		
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

void Shade::create_process()
{
	LPWSTR *argv;
	int argc;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if(argc < 2)
		error("No executable was specified");

	if(CreateProcessW(argv[1], 0, 0, 0, false, CREATE_SUSPENDED, 0, 0, &si, &pi) == 0)
		win32_error("Unable to create process");

	process = pi.hProcess;
	thread = pi.hThread;
	
	local_event_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	if(!local_event_handle)
		win32_error("Unable to create event");
	
	if(!DuplicateHandle(GetCurrentProcess(), local_event_handle, pi.hProcess, &remote_event_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
		win32_error("Unable to duplicate event handle");
}
