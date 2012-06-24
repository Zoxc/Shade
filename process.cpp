#include "process.hpp"
#include "external/heap.hpp"
#include "external/shared.hpp"
#include <shellapi.h>

HANDLE Shade::thread;
HANDLE Shade::process;
HANDLE Shade::remote_memory;
Shade::Local Shade::local;

void Shade::allocate_shared_memory()
{
	local.memory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)0x10000, 0); 

	if(!local.memory)
		win32_error("Unable to create file mapping handle");

	if(!DuplicateHandle(GetCurrentProcess(), local.memory, process, &remote_memory, 0, FALSE, DUPLICATE_SAME_ACCESS))
		win32_error("Unable to duplicate file mapping handle");

	shared = (Shared *)MapViewOfFile(local.memory, FILE_MAP_ALL_ACCESS, 0, 0, 0x1000);

	if(!shared)
		win32_error("Unable to map view of file");
	
	heap.setup((void *)(shared + 1), 0);

	create_event(local.start, shared->event_start);
	create_event(local.end, shared->event_end);
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
	
}

void Shade::signal_event(Event &event)
{
	if(!SetEvent(event.event))
		win32_error("Unable to signal event");
}

void Shade::reset_event(Event &event)
{
	if(!ResetEvent(event.event))
		win32_error("Unable to reset event");
}

void Shade::create_event(Event &local, HANDLE &remote)
{
	local.event = CreateEvent(NULL, TRUE, FALSE, NULL);
	local.thread = thread;
	
	if(!local.event)
		win32_error("Unable to create event");
	
	if(!DuplicateHandle(GetCurrentProcess(), local.event, process, &remote, 0, FALSE, DUPLICATE_SAME_ACCESS))
		win32_error("Unable to duplicate event handle");
}

void Shade::wait_event(Event &event)
{
	auto result = WaitForMultipleObjects(2, (HANDLE *)&event, FALSE, INFINITE);
	
	if(result != WAIT_OBJECT_0)
	{
		if(result == WAIT_OBJECT_0 + 1)
			error("Remote main thread terminated while wailing for event");
		else if(result == WAIT_FAILED)
			win32_error("Failed to wait for remote code");
		else
			error("Unable to wait for remote code");
	}
}

void Shade::resume_process()
{
	if(!ResumeThread(thread))
		win32_error("Unable to resume main thread");
}