#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedSortedArray.hpp"
#include "ObjectPool.hpp"

#include "ecs/types.h"
#include "ecs/entity/Entity.hpp"
#include "thread/MutexLock.hpp"

NS_ECS

class EntityManager
{
	typedef ObjectPool<Entity, ECS_MAX_ENTITY_COUNT> TPool;
	typedef std::unordered_map<Identifier, std::weak_ptr<Entity>> TAllocatedObjectMap;

public:
	EntityManager();
	~EntityManager();

	std::shared_ptr<Entity> create();
	std::shared_ptr<Entity> get(Identifier const &id) const;

private:
	thread::MutexLock mMutex;

	TPool mPool;
	TAllocatedObjectMap mAllocatedObjects;

	void destroy(Entity *pCreated);

};

NS_END
