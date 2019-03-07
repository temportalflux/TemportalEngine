#ifndef TE_NETWORK_PACKETINT_HPP
#define TE_NETWORK_PACKETINT_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Libraries ------------------------------------------------------------------
#include <string.h> // memcpy

// Engine ---------------------------------------------------------------------
#include "network/NetworkBudget.hpp"
#include "types/integer.h"

// ----------------------------------------------------------------------------
NS_NETWORK

class TEMPORTALENGINE_API PacketInternal
{
	friend class NetworkInterface;
	friend class Service;

	struct DataPtr
	{
		i8 data[MAX_PACKET_ADDRESS_LENGTH];
		uSize length;
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

	PacketInternal(DataPtr const address, DataPacket const packetData);

	template <typename T>
	T getData() const
	{
		T tmp;
		memcpy_s(&tmp, sizeof(T), this->mData.data, this->mData.length * sizeof(i8));
		return tmp;
	}

public:
	typedef ui16 Id;

	PacketInternal();

};

NS_END
// ----------------------------------------------------------------------------

#endif