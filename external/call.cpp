#include "call.hpp"

namespace Shade
{
	Call call;

	void Call::done()
	{
		SetEvent(event);
		while(true);
	}
};