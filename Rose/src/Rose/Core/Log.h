#pragma once

#include <stdio.h>
#include <string>

namespace Rose  { namespace Internals
{

	void LogMsg(const char* message, ...);

}}

#define LOG(x, ...) Rose::Internals::LogMsg(x, __VA_ARGS__)
#define ASSERT() __debugbreak()