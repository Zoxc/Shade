#include "external.hpp"
#include "shared.hpp"
#include "d3.hpp"

extern "C" void ctors();

namespace Shade
{
	namespace Remote
	{
		class IDirect3DDevice9;
		
		typedef HRESULT (__stdcall *d3d_present_t)(IDirect3DDevice9 *device, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
		
		DWORD last;
		
		void tick()
		{
			SetEvent(shared->event_start);
			
			while(true)
			{
				auto result = WaitForMultipleObjects(2, &shared->event_end, FALSE, INFINITE);

				if(result != WAIT_OBJECT_0) // The event was not triggered. This means the controlling application was terminated and we shouldn't do anything.
					return;
				
				ResetEvent(shared->event_end);
				
				switch(shared->call_type)
				{
					case Call::Continue:
						goto exit_loop;
						
					case Call::Dummy:
						break;
				}
				
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
			size_t mapping_size = 0x1000;
			shared = (Shared *)MapViewOfFile(memory, FILE_MAP_ALL_ACCESS, 0, 0, mapping_size);

			if(!shared)
				return 1;
			
			heap.setup((void *)(shared + 1), mapping_size - sizeof(Shared));
			
			HMODULE d3d9 = LoadLibrary("d3d9.dll");

			if(!d3d9)
				return 2;
				
			shared->d3d_present_offset = (size_t)d3d9 + shared->d3d_present_offset;
			shared->d3d_present = &d3d_present;
			
			D3::init();
			
			ctors();
			
			return 0;
		}
	};
};

extern "C" DWORD WINAPI init(void *memory)
{
	return Shade::Remote::init((HANDLE)memory);
}