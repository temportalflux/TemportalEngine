#include "network/packet/NetworkPacketECSReplicate.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"
#include "ecs/ECSNetworkedManager.hpp"
#include "network/NetworkInterface.hpp"
#include "utility/StringUtils.hpp"

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

ECSReplicate::ECSReplicate()
	: Packet(EPacketFlags::eReliable)
	, mReplicationType(EReplicationType::eInvalid)
	, mObjectEcsType(ecs::EType::eSystem) // systems are not supported for replication
	, mObjectTypeId(0), mObjectNetId(0)
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

ECSReplicate::EReplicationType const& ECSReplicate::replicationType() const
{
	return this->mReplicationType;
}
ecs::EType const& ECSReplicate::ecsType() const { return this->mObjectEcsType; }
uIndex const& ECSReplicate::ecsTypeId() const { return this->mObjectTypeId; }
ecs::Identifier const& ECSReplicate::objectNetId() const { return this->mObjectNetId; }

ECSReplicate& ECSReplicate::pushLink(ecs::EType type, uIndex objectTypeId, ecs::Identifier netId)
{
	assert(
		this->mObjectEcsType == ecs::EType::eEntity
		|| this->mObjectEcsType == ecs::EType::eView
	);
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

void ECSReplicate::write(Buffer &archive) const
{
	// cannot replicate ecs systems
	assert(this->mObjectEcsType != ecs::EType::eSystem);
	Packet::write(archive);
	
	network::write(archive, "replicationType", this->mReplicationType);
	network::write(archive, "objectType", this->mObjectEcsType);
	network::write(archive, "objectNetId", this->mObjectNetId);

	switch (this->mObjectEcsType)
	{
	case ecs::EType::eEntity:
	{
		this->writeLinks(archive);
		break;
	}
	case ecs::EType::eView:
	{
		network::write(archive, "typeId", this->mObjectTypeId);
		this->writeLinks(archive);
		break;
	}
	case ecs::EType::eComponent:
	{
		network::write(archive, "typeId", this->mObjectTypeId);
		this->writeFields(archive);
		break;
	}
	default: break;
	}
}

void ECSReplicate::read(Buffer &archive)
{
	Packet::read(archive);

	network::read(archive, "replicationType", this->mReplicationType);
	network::read(archive, "objectType", this->mObjectEcsType);
	network::read(archive, "objectNetId", this->mObjectNetId);

	switch (this->mObjectEcsType)
	{
	case ecs::EType::eEntity:
	{
		this->readLinks(archive);
		break;
	}
	case ecs::EType::eView:
	{
		network::read(archive, "typeId", this->mObjectTypeId);
		this->readLinks(archive);
		break;
	}
	case ecs::EType::eComponent:
	{
		network::read(archive, "typeId", this->mObjectTypeId);
		this->readFields(archive);
		break;
	}
	default: break;
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
			<< utility::StringParser<ecs::EType>::to_string(link.ecsType).c_str()
			<< ": TypeId(" << link.objectTypeId << ") ObjectNetId(" << link.netId << ")"
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
			auto pObject = manager->createObject(this->mObjectTypeId);
			manager->assignNetworkId(this->mObjectNetId, pObject->id);

		}
		else if (this->mReplicationType == EReplicationType::eDestroy)
		{
			if (this->mObjectEcsType == ecs::EType::eEntity)
			{
			}
			else if (this->mObjectEcsType == ecs::EType::eView)
			{

			}
			else if (this->mObjectEcsType == ecs::EType::eComponent)
			{

			}
		}
		else if (this->mReplicationType == EReplicationType::eUpdate)
		{
			if (this->mObjectEcsType == ecs::EType::eEntity)
			{
			}
			else if (this->mObjectEcsType == ecs::EType::eView)
			{

			}
			else if (this->mObjectEcsType == ecs::EType::eComponent)
			{

			}
		}
		else
		{
			assert(false);
		}
	}
}
