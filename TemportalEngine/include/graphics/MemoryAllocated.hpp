#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class Memory;

class MemoryAllocated
{
public:

	void configureSlot(Memory *memory);

protected:
	virtual vk::MemoryRequirements getRequirements() const = 0;

private:
	// The slot of object in `Memory#mSlots`.
	uIndex mIdxSlot;

};

NS_END
