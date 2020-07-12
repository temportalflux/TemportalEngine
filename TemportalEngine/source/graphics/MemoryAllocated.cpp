#include "graphics/MemoryAllocated.hpp"

#include "graphics/Memory.hpp"

using namespace graphics;

void MemoryAllocated::configureSlot(Memory *memory)
{
	memory->configureSlot(this->getRequirements(), this->mIdxSlot);
}
