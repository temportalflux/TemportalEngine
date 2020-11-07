#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class Memory;

class MemoryAllocated
{
public:

	void configureSlot(std::weak_ptr<Memory> memory);
	virtual void bindMemory() = 0;

protected:
	virtual vk::MemoryRequirements getRequirements() const = 0;

	std::shared_ptr<Memory> memory() const;
	uIndex memorySlot() const;

	void copyMemoryAllocatedFrom(MemoryAllocated const& other);

private:
	std::weak_ptr<Memory> mpMemory;
	// The slot of object in `Memory#mSlots`.
	uIndex mIdxSlot;

};

NS_END
