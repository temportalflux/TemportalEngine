#include "network/packet/NetworkPacketChunkReplication.hpp"

#include "game/GameInstance.hpp"
#include "network/NetworkInterface.hpp"
#include "world/WorldReplicated.hpp"
#include "world/WorldSaved.hpp"
#include "world/WorldTerrain.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(ChunkReplication)

ChunkReplication::ChunkReplication()
	: Packet(EPacketFlags::eReliable)
{
}

ChunkReplication& ChunkReplication::initializeDimension(ui32 dimId, std::vector<math::Vector3Int> const& chunkCoords)
{
	this->mDimensionId = dimId;

	auto pWorld = std::dynamic_pointer_cast<world::WorldSaved>(game::Game::Get()->world());
	auto pTerrain = pWorld->terrain(dimId);
	for (auto const& coord : chunkCoords)
	{
		ChunkEntry chunkData = {};
		chunkData.coord = coord;

		world::Chunk* pChunk = pTerrain->getChunk(coord);
		for (auto const& voxel : *pChunk)
		{
			if (!voxel.data) continue;
			chunkData.voxels.push_back(/*world::VoxelEntry*/std::make_pair(voxel.localCoord, voxel.data->id));
		}
		
		this->mChunks.push_back(std::move(chunkData));
	}
	return *this;
}

void ChunkReplication::write(Buffer &archive) const
{
	Packet::write(archive);

	network::write(archive, "dimension", this->mDimensionId);

	uSize const chunkCount = this->mChunks.size();
	network::write(archive, "chunkCount", chunkCount);

	for (uIndex idxChunk = 0; idxChunk < chunkCount; ++idxChunk)
	{
		auto const& chunkData = this->mChunks[idxChunk];

		uSize const voxelCount = chunkData.voxels.size();
		archive.setNamed(chunkData.coord.toString(), utility::formatStr("%u voxels", voxelCount));

		archive.writeRaw(chunkData.coord);

		archive.writeRaw(voxelCount);
		for (uIndex idxVoxel = 0; idxVoxel < voxelCount; ++idxVoxel)
		{
			auto const& voxelEntry = chunkData.voxels[idxVoxel];
			archive.writeRaw(voxelEntry.first);

			// TODO: this should be simplified to something like an int
			// because strings cost a relatively large amount of network space
			auto const& voxelId = voxelEntry.second;

			uSize const moduleLength = voxelId.moduleName.length();
			archive.writeRaw(moduleLength);
			for (uIndex i = 0; i < moduleLength; ++i) archive.writeRaw(voxelId.moduleName[i]);

			uSize const nameLength = voxelId.name.length();
			archive.writeRaw(nameLength);
			for (uIndex i = 0; i < nameLength; ++i) archive.writeRaw(voxelId.name[i]);

		}
	}

}

void ChunkReplication::read(Buffer &archive)
{
	Packet::read(archive);

	network::read(archive, "dimension", this->mDimensionId);

	uSize chunkCount = 0;
	network::read(archive, "chunkCount", chunkCount);

	this->mChunks.resize(chunkCount);
	for (uIndex idxChunk = 0; idxChunk < chunkCount; ++idxChunk)
	{
		auto& chunkData = this->mChunks[idxChunk];

		archive.readRaw(chunkData.coord);

		uSize voxelCount = 0;
		archive.readRaw(voxelCount);
		chunkData.voxels.resize(voxelCount);
		for (uIndex idxVoxel = 0; idxVoxel < voxelCount; ++idxVoxel)
		{
			auto& voxelEntry = chunkData.voxels[idxVoxel];
			archive.readRaw(voxelEntry.first);

			// TODO: this should be simplified to something like an int
			// because strings cost a relatively large amount of network space
			auto& voxelId = voxelEntry.second;

			uSize moduleLength = 0;
			archive.readRaw(moduleLength);
			voxelId.moduleName = std::string(moduleLength, '\0');
			for (uIndex i = 0; i < moduleLength; ++i) archive.readRaw(voxelId.moduleName[i]);

			uSize nameLength = 0;
			archive.readRaw(nameLength);
			voxelId.name = std::string(nameLength, '\0');
			for (uIndex i = 0; i < nameLength; ++i) archive.readRaw(voxelId.name[i]);

		}

		archive.setNamed(chunkData.coord.toString(), utility::formatStr("%u voxels", voxelCount));
	}
}

void ChunkReplication::process(Interface *pInterface)
{
	if (pInterface->type() == EType::eClient)
	{
		auto pWorld = std::dynamic_pointer_cast<world::WorldReplicated>(game::Game::Get()->world());
		pWorld->initializeDimension(this);
	}
}

ui32 const& ChunkReplication::dimensionId() const { return this->mDimensionId; }
ChunkReplication::ChunkList const& ChunkReplication::chunks() const { return this->mChunks; }
