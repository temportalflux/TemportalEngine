#include "Mutex.h"

#include "ThreadId.h"

Mutex const mutex_clear(Mutex *const pLock);
Mutex const mutex_invalidate(Mutex *const pLock);

inline ui8 mutexIsValid(Mutex const lock)
{
	return lock >= 0;
}

inline ui8 mutexIsLocked(Mutex const lock)
{
	return lock > 0;
}

Mutex const mutexLock(Mutex *const pLock)
{
	// wait for lock to be released, then set it
	while (mutexIsLocked(*pLock));

	// prevents race condition when 2 threads are waiting for the state
	if (*pLock == 0)
	{
		*pLock = threadID();
		return *pLock;
	}

	return -1;
}

ui8 mutexUnlock(Mutex *const pLock)
{
	// release lock if caller is owner
	const Mutex ret = *pLock;

	if (ret == threadID())
	{
		mutex_clear(pLock);
		return 1;
	}

	return 0;
}

Mutex const mutex_clear(Mutex *const pLock)
{
	*pLock = 0;
	return *pLock;
}

Mutex const mutex_invalidate(Mutex *const pLock)
{
	*pLock = -1;
	return *pLock;
}
