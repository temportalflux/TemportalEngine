#pragma once

#include "evcs/ECSNetworkedManager.hpp"

#include "dataStructures/FixedArray.hpp"
#include "dataStructures/ObjectPool.hpp"
#include "evcs/component/Component.hpp"
#include "thread/MutexLock.hpp"

FORWARD_DEF(NS_EVCS, class Core);

NS_EVCS
NS_COMPONENT

class Manager : public evcs::NetworkedManager
{

	struct TypeMetadata
	{
		std::string name;
		std::function<void(Component*)> construct;
		// The sizeof a given component for the type
		uSize size;
		uSize objectCount;
	};

public:
	Manager(Core *pCore);
	~Manager();

	template <typename TComponent>
	ComponentTypeId registerType(std::string const& name)
	{
		TComponent::TypeId = this->mRegisteredTypeCount;
		this->registerType(TComponent::TypeId, {
			name, &TComponent::construct,
			sizeof(TComponent), TComponent::MaxPoolSize,
		});
		return TComponent::TypeId;
	}

	void allocatePools();

	std::string typeName(TypeId const& typeId) const override;
	uSize typeSize(TypeId const& typeId) const;
	IEVCSObject* createObject(TypeId const& typeId, Identifier const& netId) override;
	IEVCSObject* getObject(TypeId const& typeId, Identifier const& netId) override;
	void destroyObject(TypeId const& typeId, Identifier const& netId) override;
	
	Component* create(ComponentTypeId const& typeId, bool bForceCreateNetId = false);
	Component* get(ComponentTypeId const& typeId, Identifier const& id);
	void destroy(ComponentTypeId const& typeId, Identifier const& id);

	template <typename TComponent>
	TComponent* create(bool bForceCreateNetId=false)
	{
		return dynamic_cast<TComponent*>(this->create(TComponent::TypeId, bForceCreateNetId));
	}

	template <typename TComponent>
	TComponent* get(Identifier const &id)
	{
		return dynamic_cast<TComponent*>(this->get(TComponent::TypeId, id));
	}

	// Called by entities when components are added to them
	void setComponentEntity(TypeId typeId, Identifier compId, Identifier entityId);
	std::optional<Identifier> getComponentEntityId(TypeId typeId, Identifier compId);

private:
	Core *mpCore;

	TypeMetadata mMetadataByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	/**
	 * Pointer to the giant chunk that holds all components.
	 * /Could/ have used `memory::MemoryChunk`, but we know exactly
	 * how big each portion should be and how much is needed
	 * ahead of time, so dynamic memory management is not needed here.
	 */
	void* mpPoolMemory;
	ObjectPool mPoolByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	std::map<Identifier, Component*> mAllocatedByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	std::map<Identifier, Identifier> mOwnerEntityByAllocatedId[ECS_MAX_COMPONENT_TYPE_COUNT];

	void registerType(ComponentTypeId const& id, TypeMetadata const& metadata);

};

NS_END
NS_END
