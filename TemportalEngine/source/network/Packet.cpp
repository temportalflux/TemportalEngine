#include "network/Packet.hpp"

using namespace network;

Packet::Packet()
{
}

Packet::Packet(DataPtr const address, DataPacket const packetData)
{
	mAddressSource = address;
	mData = packetData;
}
