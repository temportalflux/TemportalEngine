#include "network/packet/NetworkPacketECSReplicate.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"
#include "ecs/ECSNetworkedManager.hpp"
#include "network/NetworkInterface.hpp"
#include "utility/StringUtils.hpp"
#include "ecs/entity/Entity.hpp"
#include "ecs/view/ECView.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(ECSReplicate)

template <>
std::string utility::StringParser<ECSReplicate::EReplicationType>::to_string(ECSReplicate::EReplicationType const& v)
{
	switch (v)
	{
	case ECSReplicate::EReplicationType::eCreate: return "create";
	case ECSReplicate::EReplicationType::eUpdate: return "update";
	case ECSReplicate::EReplicationType::eDestroy: return "destroy";
	default: return "invalid";
	}
}

NS_NETWORK

void write(Buffer &buffer, std::string name, ECSReplicate::EReplicationType value)
{
	buffer.setNamed(name, utility::StringParser<ECSReplicate::EReplicationType>::to_string(value));
	buffer.writeRaw(value); \
}

void read(Buffer &buffer, std::string name, ECSReplicate::EReplicationType &value)
{
	buffer.readRaw(value);
	buffer.setNamed(name, utility::StringParser<ECSReplicate::EReplicationType>::to_string(value));
}

NS_END

std::string ecsTypeName(ecs::EType type, ecs::TypeId typeId)
{
	return ecs::Core::Get()->typeName(type, typeId);
}

ECSReplicate::ECSReplicate()
	: Packet(EPacketFlags::eReliable)
	, mReplicationType(EReplicationType::eInvalid)
	, mObjectEcsType(ecs::EType::eSystem) // systems are not supported for replication
	, mObjectTypeId(0), mObjectNetId(0)
	, mbHasOwnership(false), mbHasOwner(false), mOwnerNetId(0)
{
}

ECSReplicate& ECSReplicate::setReplicationType(EReplicationType type)
{
	this->mReplicationType = type;
	return *this;
}

ECSReplicate& ECSReplicate::setObjectEcsType(ecs::EType type)
{
	this->mObjectEcsType = type;
	return *this;
}

ECSReplicate& ECSReplicate::setObjectTypeId(uIndex typeId)
{
	assert(
		this->mObjectEcsType == ecs::EType::eComponent
		|| this->mObjectEcsType == ecs::EType::eView
	);
	this->mObjectTypeId = typeId;
	return *this;
}

ECSReplicate& ECSReplicate::setObjectNetId(ecs::Identifier const& netId)
{
	this->mObjectNetId = netId;
	return *this;
}

ECSReplicate& ECSReplicate::setOwner(std::optional<ui32> ownerNetId)
{
	this->mbHasOwnership = true;
	this->mbHasOwner = ownerNetId.has_value();
	if (ownerNetId) this->mOwnerNetId = ownerNetId.value();
	return *this;
}

ECSReplicate::EReplicationType const& ECSReplicate::replicationType() const
{
	return this->mReplicationType;
}
ecs::EType const& ECSReplicate::ecsType() const { return this->mObjectEcsType; }
uIndex const& ECSReplicate::ecsTypeId() const { return this->mObjectTypeId; }
ecs::Identifier const& ECSReplicate::objectNetId() const { return this->mObjectNetId; }

ECSReplicate& ECSReplicate::pushLink(ecs::EType type, uIndex objectTypeId, ecs::Identifier netId)
{
	// Function only allowed for entities or views
	assert(
		this->mObjectEcsType == ecs::EType::eEntity
		|| this->mObjectEcsType == ecs::EType::eView
	);
	// link type must be a view or component for entities
	assert(this->mObjectEcsType != ecs::EType::eEntity || (
		type == ecs::EType::eView || type == ecs::EType::eComponent
	));
	// link type must be component for views
	assert(this->mObjectEcsType != ecs::EType::eView || (
		type == ecs::EType::eComponent
	));
	this->mObjectLinks.push_back({
		type, objectTypeId, netId
	});
	return *this;
}

ECSReplicate& ECSReplicate::pushComponentField(uIndex byteOffset, void* data, uSize dataSize)
{
	assert(this->mObjectEcsType == ecs::EType::eComponent);
	assert(
		this->mReplicationType == EReplicationType::eCreate
		|| this->mReplicationType == EReplicationType::eUpdate
	);
	auto repData = ReplicatedData(dataSize / sizeof(ui8));
	memcpy_s(repData.data(), dataSize, data, dataSize);
	this->mComponentFields.push_back(std::make_pair(byteOffset, repData));
	return *this;
}

bool ECSReplicate::hasTypeId() const
{
	return this->mObjectEcsType == ecs::EType::eView || this->mObjectEcsType == ecs::EType::eComponent;
}

bool ECSReplicate::hasLinks() const
{
	return this->mObjectEcsType == ecs::EType::eEntity || this->mObjectEcsType == ecs::EType::eView;
}

bool ECSReplicate::hasFields() const
{
	return this->mObjectEcsType == ecs::EType::eComponent;
}

void ECSReplicate::write(Buffer &archive) const
{
	// cannot replicate ecs systems
	assert(this->mObjectEcsType != ecs::EType::eSystem);
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

void ECSReplicate::read(Buffer &archive)
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

std::string ECSReplicate::toBufferString(std::vector<ObjectLink> const& links) const
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
			<< utility::StringParser<ecs::EType>::to_string(link.ecsType).c_str()
			<< " net-id(" << link.netId << ")"
			;
	}
	return ss.str();
}

void ECSReplicate::writeLinks(Buffer &archive) const
{
	archive.setNamed("links", this->toBufferString(this->mObjectLinks));
	uSize const length = this->mObjectLinks.size();
	archive.writeRaw(length);
	archive.write((void*)this->mObjectLinks.data(), length * sizeof(ObjectLink));
}

void ECSReplicate::readLinks(Buffer &archive)
{
	uSize length = 0;
	archive.readRaw(length);
	this->mObjectLinks.resize(length);
	archive.read((void*)this->mObjectLinks.data(), length * sizeof(ObjectLink));
	archive.setNamed("links", this->toBufferString(this->mObjectLinks));
}

std::string ECSReplicate::toBufferString(std::vector<ReplicatedField> const& fields) const
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

void ECSReplicate::writeFields(Buffer &archive) const
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

void ECSReplicate::readFields(Buffer &archive)
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

ecs::NetworkedManager* getObjectManager(ecs::Core &ecs, ecs::EType type)
{
	switch (type)
	{
	case ecs::EType::eEntity: return &ecs.entities();
	case ecs::EType::eView: return &ecs.views();
	case ecs::EType::eComponent: return &ecs.components();
	default: return nullptr;
	}
}

void ECSReplicate::process(network::Interface *pInterface)
{
	if (pInterface->type().includes(EType::eServer))
	{

	}
	else if (pInterface->type() == EType::eClient)
	{
		auto& ecs = engine::Engine::Get()->getECS();
		auto* manager = getObjectManager(ecs, this->mObjectEcsType);
		if (this->mReplicationType == EReplicationType::eCreate)
		{
			if (this->mObjectEcsType == ecs::EType::eEntity)
			{
				assert(this->mObjectTypeId == 0);
			}
			auto pObject = manager->createObject(this->mObjectTypeId, this->mObjectNetId);
			ecs::Core::logger().log(
				LOG_VERBOSE, "Created replicated %s %u with net-id(%u)",
				ecs.fullTypeName(this->mObjectEcsType, this->mObjectTypeId).c_str(),
				pObject->id, this->mObjectNetId
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
				manager->destroyObject(this->mObjectTypeId, this->mObjectNetId);
			}
			else
			{
				ecs::Core::logger().log(
					LOG_VERBOSE, "Missing %s %s with net-id(%u), discarding destruction replication.",
					manager->typeName(this->mObjectTypeId).c_str(),
					utility::StringParser<ecs::EType>::to_string(this->mObjectEcsType).c_str(),
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
		}
		else
		{
			assert(false);
		}
	}
}

void ECSReplicate::linkObject(ecs::NetworkedManager* manager, ecs::IEVCSObject* pObject)
{
	auto* ecs = ecs::Core::Get();
	for (auto const& link : this->mObjectLinks)
	{
		if (this->mObjectEcsType == ecs::EType::eEntity)
		{
			auto pEntity = dynamic_cast<ecs::Entity*>(pObject);
			assert(pEntity != nullptr);
			if (link.ecsType == ecs::EType::eView)
			{
				auto pView = dynamic_cast<ecs::view::View*>(ecs->views().getObject(link.objectTypeId, link.netId));
				assert(pView != nullptr);
				pEntity->addView(link.objectTypeId, pView);
				ecs::Core::logger().log(
					LOG_VERBOSE, "Linking entity %u (net-id:%u) to view (type:%u) %u (net-id:%u)",
					pEntity->id, pEntity->netId,
					ecs->views().typeName(link.objectTypeId).c_str(), pView->id, pView->netId
				);
			}
			else if (link.ecsType == ecs::EType::eComponent)
			{
				auto pComp = dynamic_cast<ecs::component::Component*>(ecs->components().getObject(link.objectTypeId, link.netId));
				assert(pComp != nullptr);
				pEntity->addComponent(link.objectTypeId, pComp);
				ecs::Core::logger().log(
					LOG_VERBOSE, "Linking entity %u (net-id:%u) to %s component %u (net-id:%u)",
					pEntity->id, pEntity->netId,
					ecs->components().typeName(link.objectTypeId).c_str(), pComp->id, pComp->netId
				);
			}
			else
			{
				assert(false);
			}
		}
		else if (this->mObjectEcsType == ecs::EType::eView)
		{
			assert(link.ecsType == ecs::EType::eComponent);
			auto pView = dynamic_cast<ecs::view::View*>(pObject);
			assert(pView != nullptr);
			auto pComp = dynamic_cast<ecs::component::Component*>(ecs->components().getObject(link.objectTypeId, link.netId));
			assert(pComp != nullptr);
			pView->onComponentAdded(link.objectTypeId, pComp->id);
			ecs::Core::logger().log(
				LOG_VERBOSE, "Linking %s view %u (net-id:%u) to %s component %u (net-id:%u)",
				ecs->views().typeName(link.objectTypeId).c_str(), pView->id, pView->netId,
				ecs->components().typeName(link.objectTypeId).c_str(), pComp->id, pComp->netId
			);
		}
		else
		{
			assert(false);
		}
	}
}

void ECSReplicate::fillFields(ecs::NetworkedManager* manager, ecs::IEVCSObject* pObject)
{
	// TODO:
}

std::optional<ui32> ECSReplicate::owner() const
{
	return this->mbHasOwner ? std::make_optional(this->mOwnerNetId) : std::nullopt;
}
