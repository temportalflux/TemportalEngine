#ifndef TE_THREAD_MUTEX_HPP
#define TE_THREAD_MUTEX_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Namespace
// TODO: Organize Headers

#include "Mutex.h"

NS_THREAD

class TEMPORTALENGINE_API MutexLock
{
private:
	TE_Mutex mMutex;
public:
	MutexLock();
	inline bool isValid() const;
	inline bool isLocked() const;
	void lock();
	bool unlock();
};

NS_END

#endif