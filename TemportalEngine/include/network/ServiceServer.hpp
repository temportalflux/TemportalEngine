#ifndef NETWORK_SERVICE_SERVER_HPP
#define NETWORK_SERVICE_SERVER_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers

#include "network/NetworkService.hpp"

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

#endif