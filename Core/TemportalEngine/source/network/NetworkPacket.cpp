#include "network/NetworkPacket.hpp"

#include "Engine.hpp"
#include "network/NetworkInterface.hpp"

using namespace network;
using namespace network::packet;

std::vector<EPacketFlags> utility::EnumWrapper<EPacketFlags>::ALL = {
	EPacketFlags::eUnreliable,
	EPacketFlags::eNoNagle,
	EPacketFlags::eNoDelay,
	EPacketFlags::eReliable,
	EPacketFlags::eUseCurrentThread,
};

std::string utility::EnumWrapper<EPacketFlags>::to_string() const
{
	switch (value())
	{
		case EPacketFlags::eUnreliable: return "Unreliable";
		case EPacketFlags::eReliable: return "Reliable";
		case EPacketFlags::eNoNagle: return "NoNagle";
		case EPacketFlags::eNoDelay: return "NoDelay";
		case EPacketFlags::eUseCurrentThread: return "UseCurrentThread";
		default: return "invalid";
	}
}
std::string utility::EnumWrapper<EPacketFlags>::to_display_string() const { return to_string(); }

Packet::Packet(utility::Flags<EPacketFlags> flags)
	: mFlags(flags), mConnection(0)
{
}

utility::Flags<EPacketFlags> const& Packet::flags() const
{
	return this->mFlags;
}

void Packet::appendDebugLogHeader(std::stringstream &ss) const
{
	ss
		<< this->typeDisplayName().c_str()
		<< " [" << this->flags().to_string().c_str() << ']'
		<< '\n';
}

void Packet::write(Buffer &archive) const
{
	network::write(archive, "typeId", this->typeId());
}

void Packet::read(Buffer &archive)
{
	ui32 typeId;
	network::read(archive, "typeId", typeId);
	assert(typeId == this->typeId());
}

void Packet::sendToServer()
{
	auto& network = engine::Engine::Get()->networkInterface();
	if (network.type() == EType::eClient)
	{
		network.sendPackets({ network.connection() }, { this->shared_from_this() });
	}
	else if (network.type().includes(EType::eServer))
	{
		this->setSourceConnection(network.connection());
		this->process(&network);
	}
	else
	{
		assert(false);
	}
}

void Packet::send(ui32 connection)
{
	auto& network = engine::Engine::Get()->networkInterface();
	assert(network.type().includes(EType::eServer));
	network.sendPackets({ connection }, { this->shared_from_this() });
}

void Packet::sendTo(ui32 netId)
{
	auto& network = engine::Engine::Get()->networkInterface();
	assert(network.type().includes(EType::eServer));
	this->send(network.getConnectionFor(netId));
}

void Packet::broadcast(std::set<ui32> except)
{
	auto& network = engine::Engine::Get()->networkInterface();
	assert(network.type().includes(EType::eServer));
	network.sendPackets(network.connections(), { this->shared_from_this() }, except);
}
