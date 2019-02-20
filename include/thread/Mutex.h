#ifndef TE_THREAD_MUTEX_H
#define TE_THREAD_MUTEX_H

#include "types/integer.h"

//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#else // !__cplusplus
#endif // __cplusplus

typedef i32 TE_Mutex;

inline ui8 mutexIsValid(TE_Mutex const lock);
inline ui8 mutexIsLocked(TE_Mutex const lock);
TE_Mutex const mutexLock(TE_Mutex *const pLock);
ui8 mutexUnlock(TE_Mutex *const pLock);

#ifdef __cplusplus
}
#endif // __cplusplus

//-----------------------------------------------------------------------------

#endif // TE_THREAD_MUTEX_H