#include "physics/ChunkCollisionManager.hpp"

#include "utility/CollectionUtils.hpp"
#include "physics/PhysicsScene.hpp"

using namespace physics;

ChunkCollisionManager::ChunkCollisionManager(
	std::weak_ptr<physics::System> physics,
	std::weak_ptr<physics::Scene> scene
)
	: mpPhysics(physics)
	, mpScene(scene)
{
	// TODO: Voxels will need different materials based on their friction coefficients when voxels can define how slippery they are
	this->mMaterialDefault.setSystem(physics);
	this->mMaterialDefault.create();

	this->mShapeDefault.setSystem(physics);
	this->mShapeDefault
		.setMaterial(&this->mMaterialDefault)
		.setAsBox(math::Vector3(0.5f))
		.create();
}

ChunkCollisionManager::~ChunkCollisionManager()
{
	this->mChunks.clear();
}

/*
ChunkCollisionManager::ChunkCollision::ChunkCollision(ChunkCollision &&other) { *this = std::move(other); }

ChunkCollisionManager::ChunkCollision& ChunkCollisionManager::ChunkCollision::operator=(ChunkCollision &&other)
{
	this->collision = std::move(other.collision);
	return *this;
}
//*/

void ChunkCollisionManager::onLoadingChunk(math::Vector3Int const& coordinate)
{
	auto& chunk = this->mChunks.emplace_back(coordinate, ChunkCollision {}).second;
	for (auto iter = chunk.collision.begin(); iter != chunk.collision.end(); ++iter)
	{
		// NOTE: This will cause physics to degrade and eventually overflow at extreme values because the floats only support up to 32-bits.
		// This is unavoidable due to PhysX limitations.
		math::Vector3 position =
			(coordinate * CHUNK_SIDE_LENGTH).toFloat()
			+ (*iter).localCoord.toFloat()
			+ math::Vector3(0.5f) // physics shapes are always defined by their center
			;

		(*iter).data
			.setIsStatic(true)
			.setInitialTransform(position, math::Quaternion::Identity)
			.setSystem(this->mpPhysics)
			;
	}
}

void ChunkCollisionManager::onVoxelsChanged(TChangedVoxelsList const& changes)
{
	struct ChunkEntry
	{
		math::Vector3Int coord;
		uIndex idx;
	};
	std::optional<ChunkEntry> prev = std::nullopt;
	auto pScene = this->mpScene.lock();
	for (auto const& change : changes)
	{
		std::optional<game::BlockId> const& id = change.second; // TODO: At some point blocks will need to be able to specify custom collision
		world::Coordinate const& coordinate = change.first;
		
		// Find the appropriate chunk for this change
		if (!prev.has_value() || prev.value().coord != coordinate.chunk())
		{
			auto entryIdx = findChunk(coordinate.chunk());
			assert(entryIdx);
			prev = ChunkEntry {
				coordinate.chunk(), *entryIdx
			};
		}
		
		ChunkCollision &chunk = this->mChunks[prev->idx].second;
		physics::RigidBody &rigidBody = chunk.collision[coordinate.local()];
		if (id.has_value() && rigidBody.hasValue())
		{
			// TODO: destroy and create a new one if the collision is not the same
		}
		// Create the rigid body if there is no such rigid body at the coordinate
		else if (id.has_value() && !rigidBody.hasValue())
		{
			rigidBody.create();
			rigidBody.attachShape(&this->mShapeDefault);
			pScene->addActor(&rigidBody);
		}
		// Destroy the rigid body because the coordinate is now empty
		else if (!id.has_value() && rigidBody.hasValue())
		{
			rigidBody.release();
		}
	}
}

void ChunkCollisionManager::onUnloadingChunk(math::Vector3Int const& coordinate)
{
	OPTICK_EVENT()
	this->mChunks.erase(std::remove_if(
		this->mChunks.begin(), this->mChunks.end(),
		[coordinate](std::pair<math::Vector3Int, ChunkCollision> const& chunk) -> bool
		{
			return chunk.first == coordinate;
		}
	));
}

std::optional<uIndex> ChunkCollisionManager::findChunk(math::Vector3Int const& coordinate)
{
	OPTICK_EVENT()
	for (uIndex idx = 0; idx < this->mChunks.size(); ++idx)
	{
		if (this->mChunks[idx].first == coordinate) return idx;
	}
	return std::nullopt;
}
