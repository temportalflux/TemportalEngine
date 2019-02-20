#include "thread/ThreadId.h"

#ifdef _WIN32
#include <Windows.h>
extern inline i32 threadID()
{
	return GetCurrentThreadId();
}
#else
#include <sys/types.h>
extern inline a3ret threadID()
{
	return gettid();
}
#endif	// _WIN32
