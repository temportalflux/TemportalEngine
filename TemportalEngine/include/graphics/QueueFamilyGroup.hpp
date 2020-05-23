#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <optional>
#include <set>
#include <vector>

NS_GRAPHICS
enum class QueueFamily;

struct QueueFamilyGroup
{
	std::optional<ui32> idxGraphicsQueue;
	std::optional<ui32> idxPresentationQueue;

	bool hasFoundAllQueues() const;
	std::set<ui32> uniqueQueues() const;
	std::vector<ui32> allQueues() const;
	bool hasQueueFamily(QueueFamily type) const;
	std::optional<ui32> getQueueIndex(QueueFamily type) const;

};

NS_END
