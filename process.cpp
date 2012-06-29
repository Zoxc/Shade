#include "process.hpp"
#include "external/heap.hpp"
#include "external/shared.hpp"
#include <shellapi.h>

HANDLE Shade::thread;
HANDLE Shade::process;
HANDLE Shade::remote_memory;
Shade::Local Shade::local;

void Shade::open_process(DWORD process_id, DWORD thread_id)
{
	process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);

	if(!process)
		win32_error("Unable to open the remote process");

	DWORD size = MAX_PATH;
	char fullname[MAX_PATH];

	if(!QueryFullProcessImageNameA(process, 0, fullname, &size))
		win32_error("Unable to get the remote process filename");

	thread = OpenThread(SYNCHRONIZE, FALSE, thread_id);

	if(!thread)
		win32_error("Unable to open the window thread");
}

void Shade::find_process()
{
	const char *window_class = "D3 Main Window Class";
  
	HWND window = FindWindowExA(0, 0, window_class, 0);

	if(!window)
		error("Unable to find Diablo III process");

	DWORD process_id;

	DWORD thread_id = GetWindowThreadProcessId(window, &process_id);

	open_process(process_id, thread_id);
}

void Shade::set_privilege(HANDLE token, const char *privilege)
{
	TOKEN_PRIVILEGES token_privileges, prev_token_privileges;

	memset(&token_privileges, 0, sizeof(TOKEN_PRIVILEGES));

	if(!LookupPrivilegeValueA(0, privilege, &token_privileges.Privileges[0].Luid))
		win32_error("Unable to lookup privilege value");
     
	token_privileges.PrivilegeCount = 1;
	token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	DWORD dummy;

	if(!AdjustTokenPrivileges(token, FALSE, &token_privileges, sizeof(TOKEN_PRIVILEGES), &prev_token_privileges, &dummy))
		win32_error("Unable to adjust token privileges");
}

void Shade::get_debug_privileges()
{
	HANDLE token;
  
	if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &token))
	{
		DWORD err = GetLastError();

		if(err == ERROR_NO_TOKEN)
		{
			if(!ImpersonateSelf(SecurityImpersonation))
				win32_error("Unable to impersonate self");

			if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &token))
				win32_error(err, "Unable to open thread token");
		}
		else
			win32_error(err, "Unable to open thread token");
	}

	set_privilege(token, "SeDebugPrivilege");
}

void Shade::allocate_shared_memory()
{
	local.memory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Shared::mapping_size, 0); 
	
	if(!local.memory)
		win32_error("Unable to create file mapping handle");

	if(!DuplicateHandle(GetCurrentProcess(), local.memory, process, &remote_memory, 0, FALSE, DUPLICATE_SAME_ACCESS))
		win32_error("Unable to duplicate file mapping handle");

	shared = (Shared *)MapViewOfFile(local.memory, FILE_MAP_ALL_ACCESS, 0, 0, Shared::mapping_size);

	if(!shared)
		win32_error("Unable to map view of file");
	
	heap.setup((void *)(shared + 1), 0);

	create_event(local.start, shared->event_start);
	create_event(local.end, shared->event_end);
	
	if(!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), process, &shared->event_thread, 0, FALSE, DUPLICATE_SAME_ACCESS))
		win32_error("Unable to duplicate thread handle");
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
	auto result = WaitForMultipleObjects(2, &event.event, FALSE, INFINITE);
	
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