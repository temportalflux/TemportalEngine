#pragma once

#include "CoreInclude.hpp"

#include "game/GameUserIdRegistry.hpp"
#include "command/CommandRegistry.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkAddress.hpp"
#include "settings/UserInfo.hpp"
#include "world/WorldSaveData.hpp"

FORWARD_DEF(NS_NETWORK, class Interface);
FORWARD_DEF(NS_SAVE_DATA, class Instance);
FORWARD_DEF(NS_WORLD, class World);

NS_GAME

class Session : public virtual_enable_shared_from_this<Session>
{

public:
	Session();
	virtual ~Session();

	std::shared_ptr<world::World> world() const;

	std::map<ui32, utility::Guid> const& connectedUsers() const;
	bool hasConnectedUser(network::Identifier netId) const;
	bool hasConnectedUser(utility::Guid const& id) const;
	utility::Guid& findConnectedUser(network::Identifier netId);
	
	virtual void addConnectedUser(network::Identifier netId);
	virtual void removeConnectedUser(network::Identifier netId);

protected:
	game::UserIdRegistry& userRegistry();
	game::UserIdRegistry const& userRegistry() const;
	void clearConnectedUsers();

private:
	game::UserIdRegistry mUserRegistry;
	std::map<network::Identifier, utility::Guid> mConnectedUsers;

};

NS_END
