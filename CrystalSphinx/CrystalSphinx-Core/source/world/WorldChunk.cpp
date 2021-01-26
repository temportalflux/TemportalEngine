#include "world/WorldChunk.hpp"

#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "game/GameServer.hpp"
#include "game/GameClient.hpp"
#include "world/World.hpp"
#include "world/WorldTerrain.hpp"
#include "world/WorldCoordinate.hpp"
#include "registry/VoxelType.hpp"

using namespace world;

static logging::Logger ChunkLog = DeclareLog("ChunkLog", LOG_INFO);

Chunk::Chunk(
	std::weak_ptr<world::Terrain> world, math::Vector3Int coordinate
)
	: mpTerrain(world)
	, mCoordinate(coordinate)
{
}

math::Vector3Int const& Chunk::coordinate() const
{
	return this->mCoordinate;
}

void Chunk::setBlockId(math::Vector3Int const local, std::optional<game::BlockId> id)
{
	std::optional<BlockMetadata>& metadata = this->mBlockMetadata[local];
	auto oldMetadata = metadata;
	metadata.reset();
	if (id)
	{
		metadata = BlockMetadata(*id);
	}
	//this->mpTerrain.lock()->markCoordinateDirty(world::Coordinate(
	//	this->coordinate(), { (i32)local.x(), (i32)local.y(), (i32)local.z() }
	//), oldMetadata, metadata);
}

std::optional<game::BlockId> Chunk::getBlockId(math::Vector3Int const local) const
{
	auto& metadata = this->mBlockMetadata[local];
	return metadata ? std::make_optional(metadata->id) : std::nullopt;
}

void Chunk::generate()
{
	//srand(this->mpTerrain.lock()->getSeed());
	srand(0);

	// Only actually needed when re-generating the chunk so that no metadata from the previous gen is left over
	FOR_CHUNK_SIZE(i32, x) FOR_CHUNK_SIZE(i32, y) FOR_CHUNK_SIZE(i32, z) this->mBlockMetadata[{ x, y, z }] = std::nullopt;

	auto pGame = game::Game::Get();
	std::shared_ptr<game::VoxelTypeRegistry> typeRegistry;
	if (pGame->server())
	{
		typeRegistry = pGame->server()->voxelTypeRegistry();
	}
	else if (pGame->client())
	{
		typeRegistry = pGame->client()->voxelTypeRegistry();
	}
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

	/*
	for (uIndex i = 0; i < 8; ++i)
	{
		auto x = i % 2 == 0 ? rand() % CHUNK_SIDE_LENGTH : CHUNK_HALF_LENGTH;
		auto y = (rand() % (CHUNK_SIDE_LENGTH - 4)) + 4;
		auto z = i % 2 != 0 ? rand() % CHUNK_SIDE_LENGTH : CHUNK_HALF_LENGTH;

		auto id = nextId(false);
		this->setBlockId({ x, y, z }, id);
		if (id) idCount.at(*id)++;
	}
	//*/

	ChunkLog.log(LOG_INFO, "Chunk <%i, %i, %i> loaded with:", this->coordinate().x(), this->coordinate().y(), this->coordinate().z());
	for (auto const& entry : idCount)
	{
		ChunkLog.log(LOG_INFO, "- %s = %i", entry.first.to_string().c_str(), entry.second);
	}
}

Chunk& Chunk::load()
{
	// TODO: Save and load from file
	this->generate();
	this->mpTerrain.lock()->finishLoadingChunk(this);
	return *this;
}

Chunk& Chunk::readFrom(std::vector<VoxelEntry> const& voxelList)
{
	for (auto const& entry : voxelList)
	{
		this->setBlockId(entry.first, entry.second);
	}
	return *this;
}

void Chunk::createLoadHandle(EChunkLoadTier tier, ChunkHandle &out)
{
	out = ChunkHandle(this, tier);
	this->updateLoadTier(tier, 1);
}

void Chunk::releaseLoadHandle(EChunkLoadTier tier)
{
	this->updateLoadTier(tier, -1);
}

bool Chunk::shouldStayLoaded() const
{
	return this->mLoadTier < EChunkLoadTier::eInactive;
}

ui32& Chunk::loadTierCount(EChunkLoadTier tier) { return this->mChunkLoadFlags[ui32(tier)]; }

void Chunk::updateLoadTier(EChunkLoadTier tier, i32 delta)
{
	this->loadTierCount(tier) += delta;
	this->mLoadTier = EChunkLoadTier::COUNT;
	for (auto tier = EChunkLoadTier::eActive; tier < EChunkLoadTier::COUNT; tier = EChunkLoadTier(ui32(tier) + 1))
	{
		if (this->loadTierCount(tier) > 0)
		{
			this->mLoadTier = tier;
			break;
		}
	}
}

ChunkHandle::ChunkHandle() : mpChunk(nullptr) {}

ChunkHandle::ChunkHandle(Chunk* chunk, EChunkLoadTier tier) : mpChunk(chunk), mTier(tier) {}

ChunkHandle::ChunkHandle(ChunkHandle &&other) { *this = std::move(other); }

ChunkHandle& ChunkHandle::operator=(ChunkHandle &&other)
{
	this->mpChunk = other.mpChunk;
	other.mpChunk = nullptr;
	this->mTier = other.mTier;
	return *this;
}

ChunkHandle::~ChunkHandle()
{
	this->release();
}

void ChunkHandle::release()
{
	if (this->mpChunk != nullptr)
	{
		this->mpChunk->releaseLoadHandle(this->mTier);
		this->mpChunk = nullptr;
	}
}
