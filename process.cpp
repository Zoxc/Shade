#include "process.hpp"
#include "external/heap.hpp"
#include <shellapi.h>

Shade::Info *Shade::info;
Shade::Call Shade::call;
HANDLE Shade::thread;
HANDLE Shade::process;
Shade::Remote Shade::remote;
Shade::Local Shade::local;

static LARGE_INTEGER freq;
static UINT64 remote_call_time;
static size_t remote_call_count;

double Shade::avg_time_per_remote_call()
{
	return (((double)1000 * remote_call_time) / (double)freq.QuadPart) / (double)remote_call_count;
}

void Shade::allocate_shared_memory()
{
	local.memory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)0x10000, 0); 

	if(!local.memory)
		win32_error("Unable to create file mapping handle");

	if(!DuplicateHandle(GetCurrentProcess(), local.memory, process, &remote.memory, 0, FALSE, DUPLICATE_SAME_ACCESS))
		win32_error("Unable to duplicate file mapping handle");

	info = (Info *)MapViewOfFile(local.memory, FILE_MAP_ALL_ACCESS, 0, 0, 0x1000);

	if(!info)
		win32_error("Unable to map view of file");
	
	heap.setup((void *)(info + 1), 0);
	
	info->context.ContextFlags = CONTEXT_FLOATING_POINT | CONTEXT_CONTROL | CONTEXT_INTEGER;
}

void Shade::remote_event(void *ip, bool paused)
{
	LARGE_INTEGER start, stop;

	DWORD exit;

	if(!GetExitCodeProcess(process, &exit))
		win32_error("Unable to get exit code");

	if(exit != STILL_ACTIVE)
		win32_error("Process is dead!");

	QueryPerformanceCounter(&start);

	if(!ResetEvent(local.event_handle))
		win32_error("Unable to reset event");

	if(!paused)
	{
		if(SuspendThread(thread) == (DWORD)-1)
			win32_error("Unable to pause remote thread");
	}

	if(!GetThreadContext(thread, &info->context))
		win32_error("Unable to get remote thread context");
	
	CONTEXT new_context = info->context;
	
	new_context.Eip = (DWORD)ip;
	new_context.Esp -= 128; // Skip some bytes on the stack
	new_context.Esp = new_context.Esp & ~15; // Align to 16 byte boundary
		
	if(!SetThreadContext(thread, &new_context))
		win32_error("Unable to set remote thread context");
	
	if(ResumeThread(thread) == (DWORD)-1)
		win32_error("Unable to start remote code");

	if(WaitForSingleObject(local.event_handle, INFINITE) == WAIT_FAILED)
		win32_error("Unable to wait for remote code");
	
	QueryPerformanceCounter(&stop);

	remote_call_time += stop.QuadPart - start.QuadPart;
	remote_call_count++;
}

void Shade::create_process()
{
	QueryPerformanceFrequency(&freq);

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
	
	local.event_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	if(!local.event_handle)
		win32_error("Unable to create event");
	
	if(!DuplicateHandle(GetCurrentProcess(), local.event_handle, pi.hProcess, &remote.event_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
		win32_error("Unable to duplicate event handle");
}
