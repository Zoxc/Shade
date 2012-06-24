#include "d3d.hpp"
#include <d3d9.h>

void Shade::get_preset_offset()
{
	IDirect3D9 *d3d;

	if((d3d = Direct3DCreate9(D3D_SDK_VERSION)) == 0)
		error("Unable to create a DirectX context");
		
	IDirect3DDevice9 *device;

	HINSTANCE instance = GetModuleHandle(0);

	WNDCLASSEXA wndclass = {sizeof(WNDCLASSEXA), CS_CLASSDC, &DefWindowProc, 0L, 0L, instance, NULL, NULL, NULL, NULL, "WindowClass", NULL};
    
	if(RegisterClassExA(&wndclass) == 0)
		win32_error("Unable to register the window class");

	HWND window = CreateWindowA("WindowClass", "", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, GetDesktopWindow(), NULL, instance, 0);

	if(!window)
		win32_error("Unable to create a window");
	
	D3DPRESENT_PARAMETERS d3pp;
    memset(&d3pp, 0, sizeof(d3pp));
	d3pp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3pp.hDeviceWindow = window;
	d3pp.Windowed = TRUE;

	HRESULT result = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &d3pp, &device);

	if(FAILED(result))
		error("Unable to create a DirectX device");

	HMODULE d3d9 = GetModuleHandleA("d3d9.dll");

	if(!d3d9)
		win32_error("Unable to get address of d3d9.dll");

	shared->d3d_present_offset = (*(size_t **)device)[17] - (size_t)d3d9;
	
	device->Release();
	d3d->Release();

    UnregisterClassA("WindowClass", instance);
}
