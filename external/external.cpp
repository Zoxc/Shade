#include "external.hpp"
#include "shared.hpp"
#include "d3.hpp"
#include "ui.hpp"

extern "C" void ctors();

namespace Shade
{
	namespace Remote
	{
		class IDirect3DDevice9;
		
		typedef HRESULT (__stdcall *d3d_present_t)(IDirect3DDevice9 *device, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
		
		DWORD last;
		
		void set_error(Error::Type error)
		{
			shared->error_type = error;
		}
		
		void tick()
		{
			SetEvent(shared->event_start);
			
			while(true)
			{
				auto result = WaitForMultipleObjects(2, &shared->event_end, FALSE, INFINITE);

				if(result != WAIT_OBJECT_0) // The event was not triggered. This means the controlling application was terminated and we shouldn't do anything.
					return;
				
				ResetEvent(shared->event_end);
				
				heap.reset();
				
				switch(shared->call_type)
				{
					case Call::Continue:
						goto exit_loop;
						
					case Call::ListUI:
						list_ui();
						break;
						
					case Call::Dummy:
						break;
				}
				
				shared->result.num = 1234;
				
				if(shared->error_type == Error::Unknown)
					shared->error_type = Error::None;
					
				__sync_synchronize();
				
				SetEvent(shared->event_start);
			}
			
			exit_loop:;
		}

		HRESULT __stdcall d3d_present(IDirect3DDevice9 *device, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
		{
			DWORD new_tick = GetTickCount();
			
			if(new_tick - last > 2000)
			{
				tick();
				
				last = new_tick;
			}
			
			return ((d3d_present_t)shared->d3d_present)(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
		}

		size_t init(HANDLE memory)
		{
			shared = (Shared *)MapViewOfFile(memory, FILE_MAP_ALL_ACCESS, 0, 0, Shared::mapping_size);

			if(!shared)
				return GetLastError();
			
			heap.setup((void *)(shared + 1), Shared::mapping_size - sizeof(Shared));
			
			HMODULE d3d9 = LoadLibrary("d3d9.dll");

			if(!d3d9)
				return GetLastError();
				
			shared->d3d_present_offset = (size_t)d3d9 + shared->d3d_present_offset;
			shared->d3d_present = &d3d_present;
			
			D3::init();
			
			ctors();
			
			return ERROR_SUCCESS;
		}
	};
};

extern "C" DWORD WINAPI init(void *memory)
{
	return Shade::Remote::init((HANDLE)memory);
}