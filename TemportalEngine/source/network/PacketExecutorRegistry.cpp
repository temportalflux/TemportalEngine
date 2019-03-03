#include "network/PacketExecutorRegistry.hpp"

using namespace network;

PacketExecutorRegistry::PacketExecutorRegistry()
{
	memset(mPacketExecutors, 0, MAX_PACKET_COUNT * sizeof(DelegatePacketExecutor));
}

std::optional<Packet::Id> PacketExecutorRegistry::registerPacket(DelegatePacketExecutor pCallback)
{
	if (mPacketTypeCount < MAX_PACKET_COUNT)
	{
		this->mPacketExecutors[mPacketTypeCount] = pCallback;
		return std::make_optional(mPacketTypeCount++);
	}
	return std::nullopt;
}

std::optional<PacketExecutorRegistry::DelegatePacketExecutor> PacketExecutorRegistry::getPacketExecutor(Packet::Id const & packetId)
{
	if (packetId < mPacketTypeCount && mPacketExecutors[packetId] != nullptr)
		return std::make_optional(mPacketExecutors[packetId]);
	return std::nullopt;
}
