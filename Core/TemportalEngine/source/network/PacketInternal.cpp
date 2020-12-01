#include "network/PacketInternal.hpp"

using namespace network;

PacketInternal::PacketInternal()
{
}

PacketInternal::PacketInternal(DataPtr const address, DataPacket const packetData)
{
	mAddressSource = address;
	mData = packetData;
}
