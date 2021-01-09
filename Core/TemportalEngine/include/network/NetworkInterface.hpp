#pragma once

#include "TemportalEnginePCH.hpp"
#include "network/NetworkAddress.hpp"

NS_NETWORK

class Interface
{

public:
	enum class EType : ui8
	{
		eInvalid = 0,
		eClient = 1,
		eServer = 2,
	};

	Interface();

	Interface& setType(EType type);
	EType type() const { return this->mType; }
	
	Interface& setAddress(Address const& address);

	void start();
	bool hasConnection() const;
	void update(f32 deltaTime);
	void stop();

private:
	EType mType;
	// For clients: the address and port of the server to connect to
	// For servers: localhost + the port to listen on
	Address mAddress;

	void* /*ISteamNetworkingSockets*/ mpInternal;
	
	// For clients: the network connection to a server
	// For servers: the listen socket
	ui32 mConnection;
	ui32 mServerPollGroup;

	std::set<ui32> mClientIds;

	void pollIncomingMessages();

public:
	void onServerConnectionStatusChanged(void* pInfo);
	void onClientConnectionStatusChanged(void* pInfo);

};

NS_END
