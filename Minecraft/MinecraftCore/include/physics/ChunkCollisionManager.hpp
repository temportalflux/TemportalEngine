#pragma once

#include "CoreInclude.hpp"

#include "world/Events.hpp"

NS_PHYSICS

class ChunkCollisionManager : public WorldEventListener
{

private:
	void onLoadingChunk(math::Vector3Int const& coordinate) override {}
	void onUnloadingChunk(math::Vector3Int const& coordinate) override {}
	void onVoxelsChanged(TChangedVoxelsList const& changes) override {}

};

NS_END
