#ifndef TE_THREAD_MUTEX_H
#define TE_THREAD_MUTEX_H

#include "types/integer.h"

//-----------------------------------------------------------------------------

typedef i32 Mutex;

inline ui8 mutexIsValid(Mutex const lock);
inline ui8 mutexIsLocked(Mutex const lock);
Mutex const mutexLock(Mutex *const pLock);
ui8 mutexUnlock(Mutex *const pLock);

//-----------------------------------------------------------------------------

#endif // TE_THREAD_MUTEX_H