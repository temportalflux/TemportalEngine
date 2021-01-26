#include "world/WorldReplicated.hpp"

#include "network/packet/NetworkPacketChunkReplication.hpp"
#include "world/WorldTerrain.hpp"
#include "world/WorldChunk.hpp"

using namespace world;

WorldReplicated::WorldReplicated()
	: World()
{
}

void WorldReplicated::initializeDimension(network::packet::ChunkReplication *pPacket)
{
	world::DimensionId const& dimId = pPacket->dimensionId();
	auto& dimension = this->dimension(dimId);

	for (auto const& chunkData : pPacket->chunks())
	{
		auto pChunk = dimension.mpTerrain->startLoadingChunk(chunkData.coord);
		pChunk->readFrom(chunkData.voxels);
		dimension.mpTerrain->finishLoadingChunk(pChunk);
	}
}
