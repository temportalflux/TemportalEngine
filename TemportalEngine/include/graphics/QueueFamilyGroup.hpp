#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <optional>
#include <set>
#include <vector>

NS_GRAPHICS

enum class QueueFamily
{
	eGraphics,
	ePresentation,
};

struct QueueFamilyGroup
{
	std::optional<ui32> idxGraphicsQueue;
	std::optional<ui32> idxPresentationQueue;

	bool hasFoundAllQueues() const
	{
		return idxGraphicsQueue.has_value() && idxPresentationQueue.has_value();
	}

	std::set<ui32> uniqueQueues() const
	{
		return { idxGraphicsQueue.value(), idxPresentationQueue.value() };
	}

	std::vector<ui32> allQueues() const
	{
		return { idxGraphicsQueue.value(), idxPresentationQueue.value() };
	}

	bool hasQueueFamily(QueueFamily type) const
	{
		switch (type)
		{
		case QueueFamily::eGraphics: return idxGraphicsQueue.has_value();
		case QueueFamily::ePresentation: return idxPresentationQueue.has_value();
		}
		return false;
	}
};

NS_END
