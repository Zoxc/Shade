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

typedef struct d3c_error
{
	const char *message;
} *d3c_error_t;

D3C_EXPORT d3c_error_t D3C_API d3c_init();
D3C_EXPORT void D3C_API d3c_free_error(d3c_error_t error);

#ifdef __cplusplus
}
#endif