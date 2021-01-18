#pragma once

#include "game/GameSession.hpp"

#include "saveData/ServerSettings.hpp"
#include "utility/Flags.hpp"

NS_GAME

class Server : public Session
{

public:
	Server();
	~Server();

	void loadFrom(saveData::Instance *saveInstance);
	void init();

	bool hasSaveForUser(utility::Guid const& id) const;
	void initializeUser(utility::Guid const& id, crypto::RSAKey const& key);
	crypto::RSAKey getUserPublicKey(utility::Guid const& id) const;
	game::UserInfo getUserInfo(utility::Guid const& id) const;
	
	crypto::RSAKey& serverRSA() { return this->mServerRSA; }

	void setupNetwork(utility::Flags<network::EType> flags);
	void onNetworkConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId) override;
	void kick(ui32 netId);

	void associatePlayer(ui32 netId, ecs::Identifier entityId);

private:
	saveData::ServerSettings mServerSettings;
	crypto::RSAKey mServerRSA;

	void onNetworkStarted(network::Interface *pInterface);
	void onDedicatedClientAuthenticated(network::Interface *pInterface, ui32 netId);
	void onDedicatedClientDisconnected(network::Interface *pInterface, ui32 netId);
	void onNetworkConnnectionClosed(network::Interface *pInterface, ui32 connection, ui32 netId);
	void onNetworkStopped(network::Interface *pInterface);

	std::map<ui32, ecs::Identifier> mNetIdToPlayerEntityId;
	void destroyPlayer(ui32 netId);

};

NS_END
