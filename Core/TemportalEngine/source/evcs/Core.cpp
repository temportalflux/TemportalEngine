#include "evcs/Core.hpp"

#include "Engine.hpp"
#include "network/NetworkCore.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace evcs;

Core* Core::Get()
{
	return &engine::Engine::Get()->getECS();
}

logging::Logger& Core::logger()
{
	return Core::Get()->mLog;
}

Core::Core()
	: mComponentManager(this)
{
}

Core::~Core()
{
}

Core& Core::setLog(logging::Logger log)
{
	this->mLog = log;
	return *this;
}

EntityManager& Core::entities() { return this->mEntityManager; }

component::Manager& Core::components() { return this->mComponentManager; }

view::Manager& Core::views() { return this->mViewManager; }

std::string Core::typeName(EType type, TypeId typeId) const
{
	switch (type)
	{
	case EType::eEntity: return this->mEntityManager.typeName(typeId);
	case EType::eView: return this->mViewManager.typeName(typeId);
	case EType::eComponent: return this->mComponentManager.typeName(typeId);
	default: return "unknown";
	}
}

std::string Core::fullTypeName(EType type, TypeId typeId) const
{
	auto subtypeName = this->typeName(type, typeId);
	if (subtypeName.length() > 0) subtypeName += " ";
	return subtypeName + utility::StringParser<EType>::to_string(type);
}

void Core::beginReplication()
{
	assert(!this->mbIsReplicating);
	assert(this->mReplicators.size() == 0);
	auto& network = engine::Engine::Get()->networkInterface();
	if (network.isRunning()) this->mbIsReplicating = true;
}

bool Core::shouldReplicate() const
{
	return this->mbIsReplicating;
}

Core::ReplicationPacket Core::replicate()
{
	assert(this->shouldReplicate());
	auto replPacket = network::packet::EVCSReplicate::create();
	this->mReplicators.push_back(replPacket);
	return replPacket;
}

Core::ReplicationPacket Core::replicateCreate()
{
	assert(this->shouldReplicate());
	auto packet = replicate();
	packet->setReplicationType(
		network::packet::EVCSReplicate::EReplicationType::eCreate
	);
	return packet;
}

Core::ReplicationPacket Core::replicateUpdate(
	EType ecsType, TypeId typeId, Identifier netId
)
{
	assert(this->shouldReplicate());
	auto const typeCreate = network::packet::EVCSReplicate::EReplicationType::eCreate;
	auto const typeUpdate = network::packet::EVCSReplicate::EReplicationType::eUpdate;
	ReplicationPacket packet = nullptr;

	auto itPrevPacket = this->mReplicators.end();
	while (itPrevPacket > this->mReplicators.begin())
	{
		itPrevPacket--;
		auto prepRepl = std::reinterpret_pointer_cast<network::packet::EVCSReplicate>(*itPrevPacket);
		if (
			prepRepl->replicationType() != typeCreate
			&& prepRepl->replicationType() != typeUpdate)
		{
			continue;
		}
		if (prepRepl->ecsType() != ecsType) continue;
		if (prepRepl->ecsTypeId() != typeId) continue;
		if (prepRepl->objectNetId() != netId) continue;
		// If we have reached here in the filter, then we have found a packet
		// which was either creating or updating an ecs object
		// with the same ecs type (entity, view, or component)
		// and the same object type (component or view class)
		// and the same network id.
		// It is safe to pull this packet to the end of the list
		// and append data to it.

		// This moves the packet at the current iterator
		// to the end of the list (so it is the last to replicate)
		this->mReplicators.erase(itPrevPacket);
		this->mReplicators.push_back(prepRepl);
		
		packet = prepRepl;
		break;
	}

	if (packet == nullptr)
	{
		packet = replicate();
		packet->setObjectEcsType(ecsType);
		packet->setReplicationType(typeUpdate);
		packet->setObjectTypeId(typeId);
		packet->setObjectNetId(netId);
	}
	return packet;
}

Core::ReplicationPacket Core::replicateDestroy()
{
	assert(this->shouldReplicate());
	auto packet = replicate();
	packet->setReplicationType(
		network::packet::EVCSReplicate::EReplicationType::eDestroy
	);
	return packet;
}

void Core::endReplication(std::set<network::ConnectionId> const& ignoredConnections)
{
	if (this->mbIsReplicating)
	{
		this->mbIsReplicating = false;
		if (this->mReplicators.size() > 0)
		{
			auto& network = engine::Engine::Get()->networkInterface();
			network.sendPackets(network.connections(), this->mReplicators, ignoredConnections);
			this->mReplicators.clear();
		}
	}
}
