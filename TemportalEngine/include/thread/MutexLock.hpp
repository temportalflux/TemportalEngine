#ifndef TE_THREAD_MUTEX_HPP
#define TE_THREAD_MUTEX_HPP

#include "Api.h"

#include "Mutex.h"

class TEMPORTALENGINE_API TE_MutexLock
{
private:
	TE_Mutex mMutex;
public:
	TE_MutexLock();
	inline bool isValid() const;
	inline bool isLocked() const;
	void lock();
	bool unlock();
};

#endif // TE_THREAD_MUTEX_HPP