#pragma once

#include "CoreInclude.hpp"

#include "BlockId.hpp"
#include "world/WorldCoordinate.hpp"

using TOnChunkLoadingEvent = BroadcastDelegate<void(math::Vector3Int const&)>;
using TOnChunkLoadingListener = std::function<void(math::Vector3Int const&)>;

using TChangedVoxelsList = std::vector<std::pair<world::Coordinate, std::optional<game::BlockId>>>;
using TOnVoxelsChangedEvent = BroadcastDelegate<void(TChangedVoxelsList const&)>;
using TOnVoxelsChangedListener = std::function<void(TChangedVoxelsList const&)>;

class WorldEventListener
{

public:
	TOnChunkLoadingListener onLoadingChunkEvent();
	TOnChunkLoadingListener onUnloadingChunkEvent();
	TOnVoxelsChangedListener onVoxelsChangedEvent();

protected:
	virtual void onLoadingChunk(math::Vector3Int const& coordinate) {}
	virtual void onUnloadingChunk(math::Vector3Int const& coordinate) {}
	virtual void onVoxelsChanged(TChangedVoxelsList const& changes) {}

};
