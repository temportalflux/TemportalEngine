#ifndef TE_THREAD_MUTEX_HPP
#define TE_THREAD_MUTEX_HPP

#include "Mutex.h"

class MutexLock
{
private:
	Mutex mMutex;
public:
	MutexLock();
	inline bool isValid() const;
	inline bool isLocked() const;
	void lock();
	bool unlock();
};

#endif // TE_THREAD_MUTEX_HPP