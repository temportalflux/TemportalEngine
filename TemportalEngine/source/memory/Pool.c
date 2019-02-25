#include "memory/Pool.h"

#include <string.h> // memset

typedef struct a3_mem_Pool_ObjectHeader a3_mem_Pool_ObjectHeader;
typedef struct a3_mem_Pool_internal a3_mem_Pool_internal;

struct a3_mem_Pool_ObjectHeader
{
	a3_mem_Pool_ObjectHeader* prev;
	a3_mem_Pool_ObjectHeader* next; // if inUse, this is next inUse; if not inUse, this is next free

	/**
	* Denotes if the data that follows this node is currently in use by something
	*/
	char inUse;
};

struct a3_mem_Pool_internal
{
	ui32 mSizePerObject;
	ui32 mMaxCount, mCount;
	void* mNodeStart;
	a3_mem_Pool_ObjectHeader *mpHeaderFreeHead;
	a3_mem_Pool_ObjectHeader *mpHeaderUsedHead;
};

//-----------------------------------------------------------------------------
// Prototypes

void a3_mem_pool_resetObjects(a3_mem_Pool* pPool);
inline a3_mem_Pool_ObjectHeader* a3_mem_pool_getFirstHeader(a3_mem_Pool* pPool);
inline a3_mem_Pool_ObjectHeader* a3_mem_pool_getHeaderAt(a3_mem_Pool* pPool, ui32 const i);

//-----------------------------------------------------------------------------
// Public

extern inline ui32 const a3_mem_pool_getAllocationSizeContent(ui32 const maxCount, ui32 const sizePerObject)
{
	return maxCount * (sizeof(a3_mem_Pool_ObjectHeader) + sizePerObject);
}

extern a3_mem_Pool* a3_mem_pool_initialize(void* poolLocation, void* contentStart, ui32 const maxCount, ui32 const sizePerObject)
{
	a3_mem_Pool* pPool = (a3_mem_Pool*)poolLocation;
	pPool->mMaxCount = maxCount;
	pPool->mSizePerObject = sizePerObject;
	pPool->mNodeStart = contentStart;
	a3_mem_pool_resetObjects(pPool);
	return pPool;
}

extern void* a3_mem_pool_alloc(a3_mem_Pool *pPool)
{
	if (pPool->mCount >= pPool->mMaxCount) return 0;

	a3_mem_Pool_ObjectHeader* header = pPool->mpHeaderFreeHead;

	pPool->mpHeaderFreeHead = header->next;
	if (pPool->mpHeaderFreeHead != 0)
		((a3_mem_Pool_ObjectHeader*)pPool->mpHeaderFreeHead)->prev = 0;

	// Mark as in use
	header->inUse = 1;
	header->next = 0;
	header->prev = 0;

	a3_mem_Pool_ObjectHeader* prev = 0;
	// the next node in linear memory that is used
	a3_mem_Pool_ObjectHeader* nextLinearInUsed = pPool->mpHeaderUsedHead;
	while (nextLinearInUsed < header && nextLinearInUsed != 0)
	{
		prev = nextLinearInUsed;
		nextLinearInUsed = nextLinearInUsed->next;
	}

	header->next = nextLinearInUsed;
	header->prev = prev;
	if (nextLinearInUsed != 0)
		nextLinearInUsed->prev = header;
	if (prev != 0)
		prev->next = header;
	if (nextLinearInUsed == pPool->mpHeaderUsedHead)
		pPool->mpHeaderUsedHead = header;

	pPool->mCount++;

	return (void*)(header + 1);
}

extern void a3_mem_pool_dealloc(a3_mem_Pool *pPool, void* object)
{
	a3_mem_Pool_ObjectHeader *header = ((a3_mem_Pool_ObjectHeader*)object) - 1;

	if (header->prev != 0)
		header->prev->next = header->next;
	else
		pPool->mpHeaderUsedHead = header->next;

	if (header->next != 0)
		header->next->prev = header->prev;

	// Mark as free
	header->inUse = 0;
	header->next = 0;
	header->prev = 0;

	a3_mem_Pool_ObjectHeader* prev = 0;
	a3_mem_Pool_ObjectHeader* next = pPool->mpHeaderFreeHead;
	while (next < header && next != 0)
	{
		prev = next;
		next = next->next;
	}

	header->next = next;
	if (next != 0)
		next->prev = header;
	if (next == pPool->mpHeaderFreeHead)
		pPool->mpHeaderFreeHead = header;

	header->prev = prev;
	if (prev != 0)
		prev->next = header;

	pPool->mCount--;

	// Clear memory for given object
	memset(object, 0, pPool->mSizePerObject);
}

inline extern void* a3_mem_pool_getObjectAtIndex(a3_mem_Pool *pPool, ui32 i)
{
	return (void*)(a3_mem_pool_getHeaderAt(pPool, i) + 1);
}

extern ui8 a3_mem_pool_getIteratorNext(a3_mem_Pool const *pPool, void** obj, ui32 *i)
{
	a3_mem_Pool_internal *pPoolInternal = (a3_mem_Pool_internal *)pPool;

	// there is nothing allocated
	if (pPoolInternal->mCount == pPoolInternal->mMaxCount)
	{
		return 0;
	}
	// This is the first iteration (object is null)
	else if (*obj == 0)
	{
		// Get the first object
		*obj = (void*)(pPoolInternal->mpHeaderUsedHead + 1);
		// It is the first index
		*i = 0;
		return 1;
	}
	// This is some in-between iteration (next index is less than the current count)
	else if (*i + 1 < pPool->mCount)
	{
		// get the current object header
		a3_mem_Pool_ObjectHeader* header = (a3_mem_Pool_ObjectHeader*)(*obj) - 1;
		// And get the next active object
		// this should always be valid because we are not at the end of the list
		a3_mem_Pool_ObjectHeader* nextUsed = header->next;
		*obj = (void*)(nextUsed + 1);
		(*i)++;
		return 1;
	}
	// This is the last iteration
	else 
	{
		// there are no more active objects
		*obj = 0;
		// we are at the max count
		*i = pPool->mMaxCount;
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Private

void a3_mem_pool_resetObjects(a3_mem_Pool* pPool)
{
	pPool->mCount = 0;

	char* headerLocation = (char*)a3_mem_pool_getFirstHeader(pPool);
	char* prevLocation = 0;
	a3_mem_Pool_ObjectHeader* header = (a3_mem_Pool_ObjectHeader*)headerLocation;

	pPool->mpHeaderFreeHead = header;
	pPool->mpHeaderUsedHead = 0;
	for (ui32 i = 0; i < pPool->mMaxCount; i++)
	{
		header->inUse = 0;
		header->prev = (a3_mem_Pool_ObjectHeader*)prevLocation;
		if (header->prev != 0)
			header->prev->next = header;
		header->next = 0;

		void* objectLocation = header + 1;
		memset(objectLocation, 0, pPool->mSizePerObject);

		prevLocation = headerLocation;
		headerLocation += sizeof(a3_mem_Pool_ObjectHeader) + pPool->mSizePerObject;
		header = (a3_mem_Pool_ObjectHeader*)headerLocation;
	}
}

inline a3_mem_Pool_ObjectHeader* a3_mem_pool_getFirstHeader(a3_mem_Pool* pPool)
{
	return a3_mem_pool_getHeaderAt(pPool, 0);
}

inline a3_mem_Pool_ObjectHeader* a3_mem_pool_getHeaderAt(a3_mem_Pool* pPool, ui32 const i)
{
	char* nextNodeLocation = (char*)pPool->mNodeStart + i * (sizeof(a3_mem_Pool_ObjectHeader) + pPool->mSizePerObject);
	return (a3_mem_Pool_ObjectHeader*)nextNodeLocation;
}
