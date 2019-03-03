#ifndef NETWORK_SERVICE_Server_HPP
#define NETWORK_SERVICE_Server_HPP

#include "Namespace.h"
#include "Api.h"
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