#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "types/integer.h"

//-----------------------------------------------------------------------------

typedef struct a3_mem_Pool a3_mem_Pool;

struct a3_mem_Pool
{
	ui32 mSizePerObject;
	ui32 mMaxCount, mCount;
	void* mNodeStart;
	void *mpHeaderFreeHead;
	void *mpHeaderUsedHead;
};

extern inline ui32 const a3_mem_pool_getAllocationSizeContent(ui32 const maxCount, ui32 const sizePerObject);
extern a3_mem_Pool* a3_mem_pool_initialize(void* poolLocation, void* contentStart, ui32 const maxCount, ui32 const sizePerObject);
extern void* a3_mem_pool_alloc(a3_mem_Pool *pPool);
extern void a3_mem_pool_dealloc(a3_mem_Pool *pPool, void* object);
inline extern void* a3_mem_pool_getObjectAtIndex(a3_mem_Pool *pPool, ui32 i);
extern ui8 a3_mem_pool_getIteratorNext(a3_mem_Pool const *pPool, void** obj, ui32 *i);

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// !__MEM_POOL_H