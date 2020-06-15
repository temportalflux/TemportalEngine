#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "types/integer.h"

#ifdef __cplusplus
extern "C"
{
#else	// !__cplusplus
#endif	// __cplusplus

// Public functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Purpose: Determine the maximum additional memory needed for a buffer with a known number of nodes.
 * Pre: Caller must know the exact number of allocations possible.
 * Post: Returns the amount of extra space needed in the chunk to be able to track allocations.
 */
uSize a3_mem_manager_totalChunkSize(uSize nodeCount);

/**
 * Purpose: Determine the index of a pointer in the memory assuming all objects are the size `uniformSize`.
 */
uSize a3_mem_manager_indexOfPtr(void* block, void* ptr, uSize uniformSize);

/**
* Purpose: Initialize a block of memory
* Pre: Requires the memory address (void*) and size (a3_size) of an uninitialized memory block.
* Post: The memory block will be formatted. Will return 0 if successful.
*/
ui8 a3_mem_manager_init(void* block, uSize size);

/**
* Purpose: Find an appropriate gap in the provided memory block to allocate memory to.
* Pre: The block of initialized memory (initialized with a3_mem_init) & the size of the block you want to reserve.
* Post: Returns 0 and sets the allocatedLocation reference if successful, otherwise returns false and the allocatedLocation will be 0.
*/
ui8 a3_mem_manager_alloc(void* block, uSize size, void** allocatedLocation);

/**
* Purpose: Mark memory as no longer in use.
* Pre: allocatedLocation - allocatedLocation by a3_mem_alloc
* Post: The memory at the allocatedLocation will be marked as no longer in use.
*/
void a3_mem_manager_dealloc(void* block, void* allocatedLocation);

/**
* Purpose: Mark memory as no longer in use.
* Pre: allocatedLocation - allocatedLocation by a3_mem_alloc.
*    If clear is non-zero, the data associated with this location is wiped (set to 0).
* Post: The memory at the allocatedLocation will be marked as no longer in use.
*/
void a3_mem_manager_deallocAndClear(void* block, void* allocatedLocation, char clear);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif // !__MEM_MEMORYMANAGER_H