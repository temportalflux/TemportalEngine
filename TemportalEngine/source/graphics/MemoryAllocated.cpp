#include "graphics/MemoryAllocated.hpp"

#include "graphics/Memory.hpp"

using namespace graphics;

void MemoryAllocated::configureSlot(std::weak_ptr<Memory> memory)
{
	(this->mpMemory = memory).lock()->configureSlot(this->getRequirements(), this->mIdxSlot);
}

std::shared_ptr<Memory> MemoryAllocated::memory() const
{
	return this->mpMemory.lock();
}

uIndex MemoryAllocated::memorySlot() const
{
	return this->mIdxSlot;
}

void MemoryAllocated::copyMemoryAllocatedFrom(MemoryAllocated const& other)
{
	this->mpMemory = other.mpMemory;
	this->mIdxSlot = other.mIdxSlot;
}
