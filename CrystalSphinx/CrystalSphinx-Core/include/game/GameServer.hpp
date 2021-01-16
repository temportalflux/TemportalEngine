#pragma once

#include "game/GameSession.hpp"

#include "settings/ServerSettings.hpp"
#include "utility/Flags.hpp"

NS_GAME

class Server : public Session
{

public:
	Server();
	~Server();

	void init();

	bool hasSaveForUser(utility::Guid const& id) const;
	void initializeUser(utility::Guid const& id, crypto::RSAKey const& key);
	crypto::RSAKey getUserPublicKey(utility::Guid const& id) const;
	game::UserInfo getUserInfo(utility::Guid const& id) const;
	
	crypto::RSAKey& serverRSA() { return this->mServerRSA; }

	void setupNetwork(utility::Flags<network::EType> flags);
	void onNetworkConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId) override;
	void kick(ui32 netId);

private:
	game::ServerSettings mServerSettings;
	crypto::RSAKey mServerRSA;

	void onDedicatedClientAuthenticated(network::Interface *pInterface, ui32 netId);
	void onDedicatedClientDisconnected(network::Interface *pInterface, ui32 netId);
	void onNetworkConnnectionClosed(network::Interface *pInterface, ui32 connection, ui32 netId);
	void onNetworkStopped(network::Interface *pInterface);

};

NS_END
