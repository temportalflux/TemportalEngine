#include "thread/Mutex.h"

#include "thread/ThreadId.h"

TE_Mutex const mutex_clear(TE_Mutex *const pLock);
TE_Mutex const mutex_invalidate(TE_Mutex *const pLock);

inline ui8 mutexIsValid(TE_Mutex const lock)
{
	return lock >= 0;
}

inline ui8 mutexIsLocked(TE_Mutex const lock)
{
	return lock > 0;
}

TE_Mutex const mutexLock(TE_Mutex *const pLock)
{
	// wait for lock to be released, then set it
	while (mutexIsLocked(*pLock))
	{
		(void)pLock;
	}

	// prevents race condition when 2 threads are waiting for the state
	if (*pLock == 0)
	{
		*pLock = threadID();
		return *pLock;
	}

	return -1;
}

ui8 mutexUnlock(TE_Mutex *const pLock)
{
	// release lock if caller is owner
	const TE_Mutex ret = *pLock;

	if (ret == threadID())
	{
		mutex_clear(pLock);
		return 1;
	}

	return 0;
}

TE_Mutex const mutex_clear(TE_Mutex *const pLock)
{
	*pLock = 0;
	return *pLock;
}

TE_Mutex const mutex_invalidate(TE_Mutex *const pLock)
{
	*pLock = -1;
	return *pLock;
}
