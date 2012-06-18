#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#ifdef D3C_EXPORTS
#define D3C_EXPORT __declspec(dllexport)
#else
#define D3C_EXPORT __declspec(dllimport)
#endif

#define D3C_API __cdecl

D3C_EXPORT void D3C_API d3c_init();

#ifdef __cplusplus
}
#endif