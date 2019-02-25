#ifndef MEMORY_ALLOC_H
#define MEMORY_ALLOC_H

#include "types/integer.h"

void* engineAlloc(uSize size);
ui8 engineDealloc(void** ptr);

#endif