#ifndef TE_NETWORK_PACKET_HPP
#define TE_NETWORK_PACKET_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Parents --------------------------------------------------------------------
#include "registry/RegistryItem.hpp"

// ----------------------------------------------------------------------------
NS_NETWORK

class TEMPORTALENGINE_API Packet
{

public:
	virtual Packet* operator()(void* data) = 0;
	virtual void execute() const = 0;

};

NS_END
// ----------------------------------------------------------------------------

#endif