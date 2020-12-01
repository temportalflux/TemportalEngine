#ifndef TE_NETWORK_SERVICESERVER_HPP
#define TE_NETWORK_SERVICESERVER_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "network/common/Service.hpp"

// ----------------------------------------------------------------------------
NS_NETWORK

class TEMPORTALENGINE_API ServiceServer : public Service
{
	friend class engine::Engine;

protected:

	ServiceServer();

public:

	void initialize(ui16 const port, ui16 const maxClients);

};

NS_END
// ----------------------------------------------------------------------------

#endif