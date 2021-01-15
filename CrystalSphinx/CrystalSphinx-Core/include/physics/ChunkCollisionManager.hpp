#pragma once

#include "CoreInclude.hpp"

#include "world/Events.hpp"
#include "utility/CoordMap.hpp"
#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsShape.hpp"
#include "physics/PhysicsMaterial.hpp"

NS_PHYSICS
class System;
class Scene;

class ChunkCollisionManager : public WorldEventListener
{

public:
	ChunkCollisionManager(
		std::weak_ptr<physics::System> physics,
		std::weak_ptr<physics::Scene> scene
	);
	~ChunkCollisionManager();

private:
	std::weak_ptr<physics::System> mpPhysics;
	std::weak_ptr<physics::Scene> mpScene;

	struct ChunkCollision
	{
		CoordMap<physics::RigidBody, CHUNK_SIDE_LENGTH> collision;
		//ChunkCollision(ChunkCollision const& other) = delete;
		//ChunkCollision(ChunkCollision &&other);
		//ChunkCollision& operator=(ChunkCollision &&other);
	};

	std::vector<std::pair<math::Vector3Int, ChunkCollision>> mChunks;
	physics::Material mMaterialDefault;
	physics::Shape mShapeDefault;

	void onLoadingChunk(math::Vector3Int const& coordinate) override;
	void onVoxelsChanged(TChangedVoxelsList const& changes) override;
	void onUnloadingChunk(math::Vector3Int const& coordinate) override;

	std::optional<uIndex> findChunk(math::Vector3Int const& coordinate);

};

NS_END
