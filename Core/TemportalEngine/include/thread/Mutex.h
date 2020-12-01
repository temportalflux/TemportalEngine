#ifndef TE_THREAD_MUTEX_H
#define TE_THREAD_MUTEX_H

#include "Api.h"
#include "types/integer.h"

//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#else // !__cplusplus
#endif // __cplusplus

typedef i32 TE_Mutex;

TEMPORTALENGINE_API inline ui8 mutexIsValid(TE_Mutex const lock);
TEMPORTALENGINE_API inline ui8 mutexIsLocked(TE_Mutex const lock);
TEMPORTALENGINE_API TE_Mutex const mutexLock(TE_Mutex *const pLock);
TEMPORTALENGINE_API ui8 mutexUnlock(TE_Mutex *const pLock);

#ifdef __cplusplus
}
#endif // __cplusplus

//-----------------------------------------------------------------------------

#endif // TE_THREAD_MUTEX_H