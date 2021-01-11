#pragma once

#include "network/NetworkCore.hpp"

NS_NETWORK
namespace packet { class Packet; };

class PacketTypeRegistry
{
	struct EntryMeta
	{
		std::function<std::shared_ptr<packet::Packet>()> create;
	};

public:

	template <typename TPacket>
	PacketTypeRegistry& addType()
	{
		TPacket::TypeId = (ui32)this->mEntries.size();
		this->mEntries.push_back(EntryMeta { TPacket::create });
		return *this;
	}

	std::shared_ptr<packet::Packet> create(ui32 typeId)
	{
		return this->mEntries[typeId].create();
	}

private:
	std::vector<EntryMeta> mEntries;

};

NS_END
