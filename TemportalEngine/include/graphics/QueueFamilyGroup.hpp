#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"

NS_GRAPHICS

struct QueueFamilyGroup
{
	std::optional<ui32> idxGraphicsQueue;
	std::optional<ui32> idxPresentationQueue;

	bool hasFoundAllQueues() const;
	std::set<ui32> uniqueQueues() const;
	std::vector<ui32> allQueues() const;
	bool hasQueueFamily(QueueFamily::Enum type) const;
	std::optional<ui32> getQueueIndex(QueueFamily::Enum type) const;

};

NS_END
