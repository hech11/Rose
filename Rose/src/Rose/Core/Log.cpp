#include "Log.h"

#include <stdarg.h>

namespace Rose  { namespace Internals
{

	void LogMsg(const char* message, ...)
	{
		va_list args;
		va_start(args, message);
		vprintf(message, args);
	}

}}