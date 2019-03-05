#include "thread/MutexLock.hpp"

using namespace thread;

MutexLock::MutexLock()
{
	mMutex = 0;
}

inline bool MutexLock::isValid() const
{
	return (bool)mutexIsValid(mMutex);
}

inline bool MutexLock::isLocked() const
{
	return (bool)mutexIsLocked(mMutex);
}

void MutexLock::lock()
{
	mutexLock(&mMutex);
}

bool MutexLock::unlock()
{
	return (bool)mutexUnlock(&mMutex);
}
