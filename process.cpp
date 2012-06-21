#include "process.hpp"
#include <shellapi.h>

HANDLE Shade::thread;
HANDLE Shade::process;
HANDLE Shade::local_event_handle;
HANDLE Shade::remote_event_handle;

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
		throw error("No executable was specified");

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
