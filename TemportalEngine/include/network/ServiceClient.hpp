#ifndef NETWORK_SERVICE_CLIENT_HPP
#define NETWORK_SERVICE_CLIENT_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers

#include "network/NetworkService.hpp"

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

#endif