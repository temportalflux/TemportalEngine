#pragma once

#include "CoreInclude.hpp"

#include "game/GameUserIdRegistry.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkAddress.hpp"
#include "settings/UserInfo.hpp"

FORWARD_DEF(NS_NETWORK, class Interface);

NS_GAME

class Session
{

public:
	Session();
	virtual ~Session();

	std::map<ui32, utility::Guid> const& connectedUsers() const;
	bool hasConnectedUser(ui32 netId) const;
	utility::Guid& findConnectedUser(ui32 netId);

	virtual void onNetworkConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId) {}

protected:
	game::UserIdRegistry& userRegistry();
	game::UserIdRegistry const& userRegistry() const;

	virtual void addConnectedUser(ui32 netId);
	virtual void removeConnectedUser(ui32 netId);

private:
	game::UserIdRegistry mUserRegistry;
	std::map<ui32, utility::Guid> mConnectedUsers;

};

NS_END
