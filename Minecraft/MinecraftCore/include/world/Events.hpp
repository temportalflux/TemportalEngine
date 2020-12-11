#pragma once

#include "CoreInclude.hpp"

#include "BlockId.hpp"
#include "world/WorldCoordinate.hpp"

using TOnChunkLoadingEvent = BroadcastDelegate<void(math::Vector3Int const&)>;
using TOnChunkLoadingListener = std::function<void(math::Vector3Int const&)>;

using TChangedVoxelsList = std::vector<std::pair<world::Coordinate, std::optional<game::BlockId>>>;
using TOnVoxelsChangedEvent = BroadcastDelegate<void(TChangedVoxelsList const&)>;
using TOnVoxelsChangedListener = std::function<void(TChangedVoxelsList const&)>;
