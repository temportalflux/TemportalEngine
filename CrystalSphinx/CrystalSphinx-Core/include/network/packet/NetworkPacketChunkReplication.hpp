#pragma once

#include "network/NetworkPacket.hpp"

#include "BlockId.hpp"
#include "world/WorldChunk.hpp"

NS_NETWORK
NS_PACKET

class ChunkReplication : public Packet
{
	DECLARE_PACKET_TYPE(ChunkReplication)

public:
	struct ChunkEntry
	{
		math::Vector3Int coord;
		std::vector<world::VoxelEntry> voxels;
	};
	using ChunkList = std::vector<ChunkEntry>;

	ChunkReplication();

	ChunkReplication& initializeDimension(ui32 dimId, std::vector<math::Vector3Int> const& chunkCoords);

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

	ui32 const& dimensionId() const;
	ChunkList const& chunks() const;

private:
	ui32 mDimensionId;
	ChunkList mChunks;

};

NS_END
NS_END
