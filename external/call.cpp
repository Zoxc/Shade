#include "call.hpp"

extern "C" __declspec(noreturn) LONG __stdcall NtContinue(CONTEXT *context, BOOLEAN remove_alert);

namespace Shade
{
	Call call;
	Info *info;

	void Call::setup()
	{
		heap.reset();
	}
	
	void Call::done()
	{
		CONTEXT context = info->context; // Copy the context since if we get paused before NtContinue, info->context will be overwritten
		SetEvent(event);
		NtContinue(&context, FALSE);
	}
};