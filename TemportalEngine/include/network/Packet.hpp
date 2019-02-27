#ifndef NETWORK_PACKET_HPP
#define NETWORK_PACKET_HPP

#include "Namespace.h"
#include "Api.h"
#include "types/integer.h"
#include <string.h> // memcpy

NS_NETWORK

//#define MAX_PACKET_SIZE 2147483648 // 2^31
#define MAX_PACKET_DATA_LENGTH 512 // 2^31
#define MAX_PACKET_ADDRESS_LENGTH 39 // ###.###.###.### OR 8 hex numbers w/ 7 colors

class TEMPORTALENGINE_API Packet
{
	friend class NetworkInterface;

	struct DataPtr
	{
		i8 data[MAX_PACKET_ADDRESS_LENGTH];
		ui64 length;
	};

	struct DataPacket
	{
		ui8 data[MAX_PACKET_DATA_LENGTH];
		uSize length;
	};

	struct TimestampInfo
	{
		// If the timestamps were found in the packet
		bool timesLoaded = false;
		// The lcoal time that the packet was read at
		ui64 packetReadTime_local = 0;
		// The local time duration the packet took to start reading
		ui64 readDiff_local = 0;
		// The remote time that the packet was sent at
		ui64 sentTime_remote = 0;
		// The total time (local) that the packet took to be received/read
		ui64 totalTransferTime_local = 0;
	};

private:

	DataPtr mAddressSource;
	DataPacket mData;
	TimestampInfo mTimestampInfo;

	Packet(DataPtr const address, DataPacket const packetData);

	template <typename T>
	T getData() const
	{
		T tmp;
		memcpy_s(&tmp, sizeof(T), this->mData.data, this->mData.length * sizeof(i8));
		return tmp;
	}

public:
	Packet();

};

NS_END

#endif