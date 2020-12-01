#ifndef TE_NETWORK_PACKET_NEWINCOMINGCONNECTION_HPP
#define TE_NETWORK_PACKET_NEWINCOMINGCONNECTION_HPP
#pragma warning(push)
#pragma warning(disable:4251) // disable STL warnings in dll

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Parents --------------------------------------------------------------------
#include "network/common/Packet.hpp"

// ----------------------------------------------------------------------------
NS_NETWORK

class TEMPORTALENGINE_API PacketNewIncomingConnection : public Packet
{
	GENERATE_IDENTIFICATION('NICo')

public:
	Packet* operator()(void* data) override;
	void execute() const override;

};

NS_END
// ----------------------------------------------------------------------------

#pragma warning(pop)
#endif