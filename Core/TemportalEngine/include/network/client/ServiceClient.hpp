#ifndef TE_NETWORK_SERVICECLIENT_HPP
#define TE_NETWORK_SERVICECLIENT_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "network/common/Service.hpp"

// ----------------------------------------------------------------------------
NS_NETWORK

class TEMPORTALENGINE_API ServiceClient : public Service
{
	friend class engine::Engine;

protected:

	ServiceClient();

public:

	void initialize();
	void connectToServer(char const *address, ui16 const port);

};

NS_END
// ----------------------------------------------------------------------------

#endif