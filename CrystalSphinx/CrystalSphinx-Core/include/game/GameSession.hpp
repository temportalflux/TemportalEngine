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
FORWARD_DEF(NS_GAME, class World);

NS_GAME

class Session : public virtual_enable_shared_from_this<Session>
{

public:
	Session();
	virtual ~Session();

	void initializeDedicatedSave(saveData::Instance *saveInstance);
	saveData::Instance* dedicatedSave();
	std::shared_ptr<game::World> world() const;

	std::map<ui32, utility::Guid> const& connectedUsers() const;
	bool hasConnectedUser(ui32 netId) const;
	bool hasConnectedUser(utility::Guid const& id) const;
	utility::Guid& findConnectedUser(ui32 netId);

	virtual void onNetworkConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId) {}

	virtual void addConnectedUser(ui32 netId);
	virtual void removeConnectedUser(ui32 netId);

protected:
	game::UserIdRegistry& userRegistry();
	game::UserIdRegistry const& userRegistry() const;
	void clearConnectedUsers();

private:
	saveData::Instance *mpSaveInstance;
	world::SaveData mDedicatedWorldSave;
	game::UserIdRegistry mUserRegistry;
	std::map<ui32, utility::Guid> mConnectedUsers;

};

NS_END
