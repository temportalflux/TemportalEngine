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
	logging::Logger& logger() override;

	bool hasSaveForUser(utility::Guid const& id) const;
	void initializeUser(utility::Guid const& id, crypto::RSAKey const& key);
	crypto::RSAKey getUserPublicKey(utility::Guid const& id) const;
	game::UserInfo getUserInfo(utility::Guid const& id) const;
	
	crypto::RSAKey& serverRSA() { return this->mServerRSA; }

	void setupNetwork(utility::Flags<network::EType> flags);
	void kick(network::Identifier netId);

	void addPendingAuthentication(network::ConnectionId connId, utility::Guid const& userId);
	utility::Guid removePendingAuthentication(network::ConnectionId connId);
	void associatePlayer(network::Identifier netId, evcs::Identifier entityId);

private:
	saveData::ServerSettings mServerSettings;
	crypto::RSAKey mServerRSA;

	void onNetworkStarted(network::Interface *pInterface);
	void onClientConnected(network::Interface *pInterface, network::ConnectionId connId);
	void onClientAuthenticated(network::Interface *pInterface, network::Identifier netId);
	void onDedicatedClientDisconnected(network::Interface *pInterface, network::Identifier netId);
	void onNetworkConnnectionClosed(
		network::Interface *pInterface, network::ConnectionId connection,
		std::optional<network::Identifier> netId
	);
	void onNetworkStopped(network::Interface *pInterface);

	std::map<network::ConnectionId, utility::Guid> mPendingAuthentications;

	std::map<network::Identifier, evcs::Identifier> mNetIdToPlayerEntityId;
	void destroyPlayer(network::Identifier netId);

};

NS_END
