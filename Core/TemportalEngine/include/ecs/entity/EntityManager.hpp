#pragma once

#include "ecs/ECSNetworkedManager.hpp"

#include "dataStructures/ObjectPool.hpp"
#include "ecs/entity/Entity.hpp"
#include "thread/MutexLock.hpp"

NS_ECS

class EntityManager : public ecs::NetworkedManager
{
	typedef std::unordered_map<Identifier, std::shared_ptr<Entity>> TOwnedObjectMap;
	typedef std::unordered_map<Identifier, std::weak_ptr<Entity>> TAllocatedObjectMap;

public:
	EntityManager();
	~EntityManager();

	std::string typeName(TypeId const& typeId) const;
	IEVCSObject* createObject(TypeId const& typeId) override;

	/**
	 * Allocates a new entity object.
	 *
	 * The manager will retain ownership of the entity
	 * until `Entity#kill` is called (or `EntityManager#release`.
	 * When there are no more references to the entity ptr, the entity will be removed.
	 *
	 * You can ensure that the entity is __not__ held onto by the manager
	 * by calling `Entity#kill` immediately after creation.
	 * This will result in the entity pointer being owned solely by you
	 * and only when that shared_ptr goes out of scope will the entity be destroyed.
	 * 
	 * Even if the manager does not have ownership of the entity,
	 * `EntityManager#get` can still be called to get an entity by its id.
	 */
	std::shared_ptr<Entity> create();
	std::shared_ptr<Entity> get(Identifier const &id) const;
	void release(Identifier const& id);
	void releaseAll();

	std::shared_ptr<Entity> getNetworked(Identifier const& netId) const;

private:
	thread::MutexLock mMutex;

	ObjectPool mPool;
	TOwnedObjectMap mOwnedObjects;
	TAllocatedObjectMap mAllocatedObjects;
	
	void destroy(Entity *pCreated);

};

NS_END
