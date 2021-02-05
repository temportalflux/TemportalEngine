#include "network/packet/NetworkPacketECSReplicate.hpp"

#include "Engine.hpp"
#include "evcs/Core.hpp"
#include "evcs/ECSNetworkedManager.hpp"
#include "network/NetworkInterface.hpp"
#include "utility/StringUtils.hpp"
#include "evcs/entity/Entity.hpp"
#include "evcs/view/ECView.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(EVCSReplicate)

template <>
std::string utility::StringParser<EVCSReplicate::EReplicationType>::to_string(EVCSReplicate::EReplicationType const& v)
{
	switch (v)
	{
	case EVCSReplicate::EReplicationType::eCreate: return "create";
	case EVCSReplicate::EReplicationType::eUpdate: return "update";
	case EVCSReplicate::EReplicationType::eDestroy: return "destroy";
	default: return "invalid";
	}
}

NS_NETWORK

void write(Buffer &buffer, std::string name, EVCSReplicate::EReplicationType value)
{
	buffer.setNamed(name, utility::StringParser<EVCSReplicate::EReplicationType>::to_string(value));
	buffer.writeRaw(value); \
}

void read(Buffer &buffer, std::string name, EVCSReplicate::EReplicationType &value)
{
	buffer.readRaw(value);
	buffer.setNamed(name, utility::StringParser<EVCSReplicate::EReplicationType>::to_string(value));
}

NS_END

std::string ecsTypeName(evcs::EType type, evcs::TypeId typeId)
{
	return evcs::Core::Get()->typeName(type, typeId);
}

EVCSReplicate::EVCSReplicate()
	: Packet(EPacketFlags::eReliable)
	, mReplicationType(EReplicationType::eInvalid)
	, mObjectEcsType(evcs::EType::eSystem) // systems are not supported for replication
	, mObjectTypeId(0), mObjectNetId(0)
	, mbHasOwnership(false), mbHasOwner(false), mOwnerNetId(0)
{
}

EVCSReplicate& EVCSReplicate::setReplicationType(EReplicationType type)
{
	this->mReplicationType = type;
	return *this;
}

EVCSReplicate& EVCSReplicate::setObjectEcsType(evcs::EType type)
{
	this->mObjectEcsType = type;
	return *this;
}

EVCSReplicate& EVCSReplicate::setObjectTypeId(uIndex typeId)
{
	assert(
		this->mObjectEcsType == evcs::EType::eComponent
		|| this->mObjectEcsType == evcs::EType::eView
	);
	this->mObjectTypeId = typeId;
	return *this;
}

EVCSReplicate& EVCSReplicate::setObjectNetId(evcs::Identifier const& netId)
{
	this->mObjectNetId = netId;
	return *this;
}

EVCSReplicate& EVCSReplicate::setOwner(std::optional<ui32> ownerNetId)
{
	this->mbHasOwnership = true;
	this->mbHasOwner = ownerNetId.has_value();
	if (ownerNetId) this->mOwnerNetId = ownerNetId.value();
	return *this;
}

EVCSReplicate::EReplicationType const& EVCSReplicate::replicationType() const
{
	return this->mReplicationType;
}
evcs::EType const& EVCSReplicate::ecsType() const { return this->mObjectEcsType; }
uIndex const& EVCSReplicate::ecsTypeId() const { return this->mObjectTypeId; }
evcs::Identifier const& EVCSReplicate::objectNetId() const { return this->mObjectNetId; }

EVCSReplicate& EVCSReplicate::pushLink(evcs::EType type, uIndex objectTypeId, evcs::Identifier netId)
{
	// Function only allowed for entities or views
	assert(
		this->mObjectEcsType == evcs::EType::eEntity
		|| this->mObjectEcsType == evcs::EType::eView
	);
	// link type must be a view or component for entities
	assert(this->mObjectEcsType != evcs::EType::eEntity || (
		type == evcs::EType::eView || type == evcs::EType::eComponent
	));
	// link type must be component for views
	assert(this->mObjectEcsType != evcs::EType::eView || (
		type == evcs::EType::eComponent
	));
	this->mObjectLinks.push_back({
		type, objectTypeId, netId
	});
	return *this;
}

EVCSReplicate& EVCSReplicate::pushComponentField(uIndex byteOffset, void* data, uSize dataSize)
{
	assert(this->mObjectEcsType == evcs::EType::eComponent);
	assert(
		this->mReplicationType == EReplicationType::eCreate
		|| this->mReplicationType == EReplicationType::eUpdate
	);
	// Check fields currently being replicated.
	// If the byteOffset + data size current exists, we don't need to push the field again.
	ReplicatedData* pData = nullptr;
	for (auto iter = this->mComponentFields.rbegin(); iter != this->mComponentFields.rend(); ++iter)
	{
		if (iter->first == byteOffset && iter->second.size() * sizeof(ui8) == dataSize)
		{
			pData = &(iter->second);
			break;
		}
	}
	// Field not already found, add it to the list
	if (pData == nullptr)
	{
		auto iter = this->mComponentFields.insert(
			this->mComponentFields.end(),
			std::make_pair(byteOffset, ReplicatedData(dataSize / sizeof(ui8)))
		);
		pData = &(iter->second);
	}
	// Copy the changed data into the replicating fields
	memcpy_s(pData->data(), dataSize, data, dataSize);
	return *this;
}

bool EVCSReplicate::hasTypeId() const
{
	return this->mObjectEcsType == evcs::EType::eView || this->mObjectEcsType == evcs::EType::eComponent;
}

bool EVCSReplicate::hasLinks() const
{
	return this->mObjectEcsType == evcs::EType::eEntity || this->mObjectEcsType == evcs::EType::eView;
}

bool EVCSReplicate::hasFields() const
{
	return this->mObjectEcsType == evcs::EType::eComponent;
}

void EVCSReplicate::write(Buffer &archive) const
{
	// cannot replicate ecs systems
	assert(this->mObjectEcsType != evcs::EType::eSystem);
	Packet::write(archive);
	
	network::write(archive, "replicationType", this->mReplicationType);
	network::write(archive, "ecsType", this->mObjectEcsType);

	if (this->hasTypeId())
	{
		archive.setNamed("typeId", ecsTypeName(this->mObjectEcsType, this->mObjectTypeId));
		archive.writeRaw(this->mObjectTypeId);
	}

	network::write(archive, "objectNetId", this->mObjectNetId);

	network::write(archive, "hasOwnership", this->mbHasOwnership);
	if (this->mbHasOwnership)
	{
		network::write(archive, "hasOwner", this->mbHasOwner);
		if (this->mbHasOwner)
		{
			network::write(archive, "ownerNetId", this->mOwnerNetId);
		}
	}

	if (this->hasLinks())
	{
		this->writeLinks(archive);
	}

	if (this->hasFields())
	{
		this->writeFields(archive);
	}
}

void EVCSReplicate::read(Buffer &archive)
{
	Packet::read(archive);

	network::read(archive, "replicationType", this->mReplicationType);
	network::read(archive, "ecsType", this->mObjectEcsType);

	if (this->hasTypeId())
	{
		archive.readRaw(this->mObjectTypeId);
		archive.setNamed("typeId", ecsTypeName(this->mObjectEcsType, this->mObjectTypeId));
	}

	network::read(archive, "objectNetId", this->mObjectNetId);

	network::read(archive, "hasOwnership", this->mbHasOwnership);
	if (this->mbHasOwnership)
	{
		network::read(archive, "hasOwner", this->mbHasOwner);
		if (this->mbHasOwner)
		{
			network::read(archive, "ownerNetId", this->mOwnerNetId);
		}
	}

	if (this->hasLinks())
	{
		this->readLinks(archive);
	}

	if (this->hasFields())
	{
		this->readFields(archive);
	}
}

std::string EVCSReplicate::toBufferString(std::vector<ObjectLink> const& links) const
{
	uSize const length = this->mObjectLinks.size();
	std::stringstream ss;
	ss << length;
	for (uIndex i = 0; i < length; ++i)
	{
		auto const& link = this->mObjectLinks[i];
		ss << '\n'
			<< "  - "
			<< ecsTypeName(link.ecsType, link.objectTypeId).c_str()
			<< " "
			<< utility::StringParser<evcs::EType>::to_string(link.ecsType).c_str()
			<< " net-id(" << link.netId << ")"
			;
	}
	return ss.str();
}

void EVCSReplicate::writeLinks(Buffer &archive) const
{
	archive.setNamed("links", this->toBufferString(this->mObjectLinks));
	uSize const length = this->mObjectLinks.size();
	archive.writeRaw(length);
	archive.write((void*)this->mObjectLinks.data(), length * sizeof(ObjectLink));
}

void EVCSReplicate::readLinks(Buffer &archive)
{
	uSize length = 0;
	archive.readRaw(length);
	this->mObjectLinks.resize(length);
	archive.read((void*)this->mObjectLinks.data(), length * sizeof(ObjectLink));
	archive.setNamed("links", this->toBufferString(this->mObjectLinks));
}

std::string EVCSReplicate::toBufferString(std::vector<ReplicatedField> const& fields) const
{
	uSize const length = this->mComponentFields.size();
	std::stringstream ss;
	ss << length << " fields";
	for (uIndex i = 0; i < length; ++i)
	{
		auto const& field = this->mComponentFields[i];
		ss << '\n'
			<< "  - offset(" << field.first << ") "
			<< (field.second.size() * sizeof(ui8)) << " bytes"
			;
	}
	return ss.str();
}

void EVCSReplicate::writeFields(Buffer &archive) const
{
	archive.setNamed("fields", this->toBufferString(this->mComponentFields));
	uSize const fieldCount = this->mComponentFields.size();
	archive.writeRaw(fieldCount);
	for (uIndex idxField = 0; idxField < fieldCount; ++idxField)
	{
		auto const& field = this->mComponentFields[idxField];
		// write the byte offset of the field
		archive.writeRaw(field.first);
		// write the byte-count of the field data (the length of the byte vector)
		archive.writeRaw(field.second.size());
		// write the field data (the byte vector)
		archive.write((void*)field.second.data(), field.second.size() * sizeof(ui8));
	}
}

void EVCSReplicate::readFields(Buffer &archive)
{
	uSize fieldCount = 0;
	archive.readRaw(fieldCount);
	this->mComponentFields.resize(fieldCount);
	for (uIndex idxField = 0; idxField < fieldCount; ++idxField)
	{
		auto& field = this->mComponentFields[idxField];
		// read the byte offset of the field
		archive.readRaw(field.first);
		// read the byte-count of the field data (the length of the byte vector)
		uSize dataByteCount = 0;
		archive.readRaw(dataByteCount);
		field.second.resize(dataByteCount);
		// read the field data (the byte vector)
		archive.read((void*)field.second.data(), field.second.size() * sizeof(ui8));
	}
	archive.setNamed("fields", this->toBufferString(this->mComponentFields));
}

evcs::NetworkedManager* getObjectManager(evcs::Core &ecs, evcs::EType type)
{
	switch (type)
	{
	case evcs::EType::eEntity: return &ecs.entities();
	case evcs::EType::eView: return &ecs.views();
	case evcs::EType::eComponent: return &ecs.components();
	default: return nullptr;
	}
}

void EVCSReplicate::process(network::Interface *pInterface)
{
	if (pInterface->type().includes(EType::eServer))
	{
		auto& ecs = engine::Engine::Get()->getECS();
		auto* manager = getObjectManager(ecs, this->mObjectEcsType);
		if (this->mReplicationType == EReplicationType::eUpdate)
		{
			auto pObject = manager->getObject(this->mObjectTypeId, this->mObjectNetId);
			auto const bIsOwnedBySender = pObject->owner() == pInterface->getNetIdFor(this->connection());
			if (bIsOwnedBySender)
			{
				if (this->hasFields())
				{
					this->fillFields(manager, pObject);
				}
				pObject->validate();
			}
		}
	}
	else if (pInterface->type() == EType::eClient)
	{
		auto& ecs = engine::Engine::Get()->getECS();
		auto* manager = getObjectManager(ecs, this->mObjectEcsType);
		if (this->mReplicationType == EReplicationType::eCreate)
		{
			if (this->mObjectEcsType == evcs::EType::eEntity)
			{
				assert(this->mObjectTypeId == 0);
			}
			auto pObject = manager->createObject(this->mObjectTypeId, this->mObjectNetId);
			evcs::Core::logger().log(
				LOG_VERBOSE, "Created replicated %s id(%u) with netId(%u)",
				ecs.fullTypeName(this->mObjectEcsType, this->mObjectTypeId).c_str(),
				pObject->id(), pObject->netId()
			);

			if (this->hasLinks())
			{
				this->linkObject(manager, pObject);
			}
			if (this->hasFields())
			{
				this->fillFields(manager, pObject);
			}
			if (this->mbHasOwnership)
			{
				pObject->setOwner(this->owner());
			}

			pObject->onReplicateCreate();
		}
		else if (this->mReplicationType == EReplicationType::eDestroy)
		{
			/*
				This guard is here for situations like entities with views and components
				being destroyed, causing the views and components to be destroyed too.

				`EntityManager#destroy` is organized such that entity destruction
				replication happens first, followed by the destruction
				packets of views and components.
				
				The entity destruction, however, will cause the manager to also destroy
				the views and components of the local entity, so this guard is here to
				ensure that the view and component destruction replication
				doesn't try to destroy twice.
			*/
			if (manager->hasNetworkId(this->mObjectNetId))
			{
				{
					auto pObject = manager->getObject(this->mObjectTypeId, this->mObjectNetId);
					pObject->onReplicateDestroy();
				}
				manager->destroyObject(this->mObjectTypeId, this->mObjectNetId);
			}
			else
			{
				evcs::Core::logger().log(
					LOG_VERBOSE, "Missing %s %s with net-id(%u), discarding destruction replication.",
					manager->typeName(this->mObjectTypeId).c_str(),
					utility::StringParser<evcs::EType>::to_string(this->mObjectEcsType).c_str(),
					this->mObjectNetId
				);
			}
		}
		else if (this->mReplicationType == EReplicationType::eUpdate)
		{
			auto pObject = manager->getObject(this->mObjectTypeId, this->mObjectNetId);
			if (this->hasLinks())
			{
				this->linkObject(manager, pObject);
			}
			if (this->hasFields())
			{
				this->fillFields(manager, pObject);
			}
			if (this->mbHasOwnership)
			{
				pObject->setOwner(this->owner());
			}

			pObject->onReplicateUpdate();
		}
		else
		{
			assert(false);
		}
	}
}

void EVCSReplicate::linkObject(evcs::NetworkedManager* manager, evcs::IEVCSObject* pObject)
{
	auto* ecs = evcs::Core::Get();
	for (auto const& link : this->mObjectLinks)
	{
		if (this->mObjectEcsType == evcs::EType::eEntity)
		{
			auto pEntity = dynamic_cast<evcs::Entity*>(pObject);
			assert(pEntity != nullptr);
			if (link.ecsType == evcs::EType::eView)
			{
				auto pView = dynamic_cast<evcs::view::View*>(ecs->views().getObject(link.objectTypeId, link.netId));
				assert(pView != nullptr);
				pEntity->addView(link.objectTypeId, pView);
				evcs::Core::logger().log(
					LOG_VERBOSE, "Linking entity %u (net-id:%u) to view (type:%u) %u (net-id:%u)",
					pEntity->id(), pEntity->netId(),
					ecs->views().typeName(link.objectTypeId).c_str(),
					pView->id(), pView->netId()
				);
			}
			else if (link.ecsType == evcs::EType::eComponent)
			{
				auto pComp = dynamic_cast<evcs::component::Component*>(ecs->components().getObject(link.objectTypeId, link.netId));
				assert(pComp != nullptr);
				pEntity->addComponent(link.objectTypeId, pComp);
				evcs::Core::logger().log(
					LOG_VERBOSE, "Linking entity %u (net-id:%u) to %s component %u (net-id:%u)",
					pEntity->id(), pEntity->netId(),
					ecs->components().typeName(link.objectTypeId).c_str(),
					pComp->id(), pComp->netId()
				);
			}
			else
			{
				assert(false);
			}
		}
		else if (this->mObjectEcsType == evcs::EType::eView)
		{
			assert(link.ecsType == evcs::EType::eComponent);
			auto pView = dynamic_cast<evcs::view::View*>(pObject);
			assert(pView != nullptr);
			auto pComp = dynamic_cast<evcs::component::Component*>(ecs->components().getObject(link.objectTypeId, link.netId));
			assert(pComp != nullptr);
			pView->onComponentAdded(link.objectTypeId, pComp->id());
			evcs::Core::logger().log(
				LOG_VERBOSE, "Linking %s view %u (net-id:%u) to %s component %u (net-id:%u)",
				ecs->views().typeName(link.objectTypeId).c_str(), pView->id(), pView->netId(),
				ecs->components().typeName(link.objectTypeId).c_str(), pComp->id(), pComp->netId()
			);
		}
		else
		{
			assert(false);
		}
	}
}

void EVCSReplicate::fillFields(evcs::NetworkedManager* manager, evcs::IEVCSObject* pObject)
{
	for (auto const& field : this->mComponentFields)
	{
		uSize fieldSize = field.second.size() * sizeof(ui8);
		uSize head = (uSize)pObject;
		uSize fieldPtrStart = head + field.first;
		void* fieldPtr = (void*)fieldPtrStart;
		memcpy_s(fieldPtr, fieldSize, (void*)field.second.data(), fieldSize);
	}
}

std::optional<ui32> EVCSReplicate::owner() const
{
	return this->mbHasOwner ? std::make_optional(this->mOwnerNetId) : std::nullopt;
}
