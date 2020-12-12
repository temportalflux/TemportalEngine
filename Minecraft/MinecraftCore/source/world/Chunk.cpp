#include "world/Chunk.hpp"

#include "Engine.hpp"
#include "Game.hpp"
#include "world/World.hpp"
#include "world/WorldCoordinate.hpp"
#include "registry/VoxelType.hpp"

static logging::Logger ChunkLog = DeclareLog("ChunkLog");

WorldChunk::WorldChunk(
	std::weak_ptr<world::World> world, math::Vector3Int coordinate
)
	: mpWorld(world)
	, mCoordinate(coordinate)
{
}

math::Vector3Int const& WorldChunk::coordinate() const
{
	return this->mCoordinate;
}

// TODO: Save and load from file
void WorldChunk::load()
{
	// Only actually needed when re-generating the chunk so that no metadata from the previous gen is left over
	FOR_CHUNK_SIZE(i32, x) FOR_CHUNK_SIZE(i32, y) FOR_CHUNK_SIZE(i32, z) this->mBlockMetadata[{ x, y, z }] = std::nullopt;

	auto typeRegistry = game::Game::Get()->voxelTypeRegistry();
	auto allVoxelIdsSet = typeRegistry->getIds();

	auto allVoxelIdOptions = std::vector<std::optional<game::BlockId>>();
	allVoxelIdOptions.push_back(std::nullopt);
	std::transform(
		std::begin(allVoxelIdsSet), std::end(allVoxelIdsSet),
		std::back_inserter(allVoxelIdOptions),
		[](game::BlockId const& id) { return std::optional<game::BlockId>(id); }
	);

	auto nextId = [allVoxelIdOptions](bool bIncludeNone) -> std::optional<game::BlockId>
	{
		uIndex min = bIncludeNone ? 0 : 1;
		uIndex max = allVoxelIdOptions.size();
		uIndex idIdx = (rand() % (max - min)) + min;
		return allVoxelIdOptions[idIdx];
	};

	auto idCount = std::unordered_map<game::BlockId, uSize>();
	for (auto const& id : allVoxelIdsSet)
	{
		idCount.insert(std::make_pair(id, 0));
	}

	for (i32 x = 0; x < CHUNK_SIDE_LENGTH; ++x)
	{
		for (i32 z = 0; z < CHUNK_SIDE_LENGTH; ++z)
		{
			auto id = nextId(true);
			this->setBlockId({ x, 0, z }, id);
			if (id) idCount.at(*id)++;
		}
	}

	for (uIndex i = 0; i < 8; ++i)
	{
		auto x = i % 2 == 0 ? rand() % CHUNK_SIDE_LENGTH : CHUNK_HALF_LENGTH;
		auto y = (rand() % (CHUNK_SIDE_LENGTH - 4)) + 4;
		auto z = i % 2 != 0 ? rand() % CHUNK_SIDE_LENGTH : CHUNK_HALF_LENGTH;

		auto id = nextId(false);
		this->setBlockId({ x, y, z }, id);
		if (id) idCount.at(*id)++;
	}

	ChunkLog.log(LOG_INFO, "Chunk <%i, %i, %i> loaded with:", this->coordinate().x(), this->coordinate().y(), this->coordinate().z());
	for (auto const& entry : idCount)
	{
		ChunkLog.log(LOG_INFO, "- %s = %i", entry.first.to_string().c_str(), entry.second);
	}

	this->mpWorld.lock()->onLoadedChunk(*this);
}

void WorldChunk::setBlockId(math::Vector3Int const local, std::optional<game::BlockId> id)
{
	std::optional<BlockMetadata>& metadata = this->mBlockMetadata[local];
	auto oldMetadata = metadata;
	metadata.reset();
	if (id)
	{
		metadata = BlockMetadata(*id);
	}
	//this->mpWorld.lock()->markCoordinateDirty(world::Coordinate(
	//	this->coordinate(), { (i32)local.x(), (i32)local.y(), (i32)local.z() }
	//), oldMetadata, metadata);
}
