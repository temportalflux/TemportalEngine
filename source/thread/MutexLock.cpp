#include "thread/MutexLock.hpp"

TE_MutexLock::TE_MutexLock()
{
	mMutex = 0;
}

inline bool TE_MutexLock::isValid() const
{
	return (bool)mutexIsValid(mMutex);
}

inline bool TE_MutexLock::isLocked() const
{
	return (bool)mutexIsLocked(mMutex);
}

void TE_MutexLock::lock()
{
	mutexLock(&mMutex);
}

bool TE_MutexLock::unlock()
{
	return false;// (bool)mutexUnlock(&mMutex);
}
