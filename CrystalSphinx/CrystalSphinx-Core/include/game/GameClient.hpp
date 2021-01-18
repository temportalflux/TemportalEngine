#pragma once

#include "game/GameSession.hpp"

#include "game/GameUserIdRegistry.hpp"
#include "input/Event.hpp"
#include "settings/ClientSettings.hpp"
#include "ui/Core.hpp"

class Window;
NS_ECS
FORWARD_DEF(NS_SYSTEM, class RenderEntities);
FORWARD_DEF(NS_SYSTEM, class UpdateCameraPerspective);
FORWARD_DEF(NS_SYSTEM, class MovePlayerByInput);
NS_END
FORWARD_DEF(NS_GAME, class VoxelModelManager);
FORWARD_DEF(NS_GRAPHICS, class ChunkBoundaryRenderer);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class EntityInstanceBuffer);
FORWARD_DEF(NS_GRAPHICS, class ImmediateRenderSystem);
FORWARD_DEF(NS_GRAPHICS, class RenderedString);
FORWARD_DEF(NS_GRAPHICS, class SimpleLineRenderer);
FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);
FORWARD_DEF(NS_GRAPHICS, class TextureRegistry);
FORWARD_DEF(NS_GRAPHICS, class UIRenderer);
FORWARD_DEF(NS_GRAPHICS, class VoxelGridRenderer);
FORWARD_DEF(NS_RESOURCE, class PackManager);
FORWARD_DEF(NS_SAVE_DATA, class Instance);
FORWARD_DEF(NS_UI, class FontOwner);
FORWARD_DEF(NS_UI, class DebugHUD);
FORWARD_DEF(NS_UI, class TextLogMenu);
FORWARD_DEF(NS_WORLD, class BlockInstanceBuffer);

NS_GAME

class Client : public Session
{

public:
	Client();

	std::shared_ptr<Window> getWindow();
	std::shared_ptr<graphics::ImmediateRenderSystem> renderer();
	std::shared_ptr<graphics::SkinnedModelManager> modelManager();
	std::shared_ptr<graphics::EntityInstanceBuffer> entityInstances();
	std::shared_ptr<graphics::TextureRegistry> textureRegistry();
	std::shared_ptr<ui::FontOwner> uiFontOwner();
	std::shared_ptr<ui::TextLogMenu> chat();
	game::ClientSettings& settings();

	void init();
	void uninit();

	void setupNetwork(network::Address const& serverAddress);
	void onLocalServerConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId);

	utility::Guid const& localUserId() const;
	ui32 localUserNetId() const;
	crypto::RSAKey localUserAuthKey() const;
	void setLocalUserNetId(ui32 netId);

	game::UserInfo& getConnectedUserInfo(ui32 netId);

protected:
	void addConnectedUser(ui32 netId) override;
	void removeConnectedUser(ui32 netId) override;

private:
	game::ClientSettings mClientSettings;

	std::optional<utility::Guid> mLocalUserId;
	bool hasAccount() const;
	game::UserInfo localUserInfo() const;
	std::optional<ui32> mLocalUserNetId;
	std::map<ui32, game::UserInfo> mConnectedUserInfo;
	ecs::Identifier mLocalPlayerEntityId;

	std::shared_ptr<resource::PackManager> mpResourcePackManager;

	std::shared_ptr<Window> mpWindow;
	std::shared_ptr<graphics::ImmediateRenderSystem> mpRenderer;

	std::shared_ptr<game::VoxelModelManager> mpVoxelModelManager;
	std::shared_ptr<world::BlockInstanceBuffer> mpVoxelInstanceBuffer;
	
	std::shared_ptr<graphics::VoxelGridRenderer> mpVoxelGridRenderer;
	std::shared_ptr<graphics::SimpleLineRenderer> mpWorldAxesRenderer;
	std::shared_ptr<graphics::ChunkBoundaryRenderer> mpChunkBoundaryRenderer;
	std::shared_ptr<graphics::UIRenderer> mpUIRenderer;
	
	std::shared_ptr<graphics::SkinnedModelManager> mpSkinnedModelManager;
	std::shared_ptr<graphics::EntityInstanceBuffer> mpEntityInstanceBuffer;
	std::shared_ptr<ecs::system::RenderEntities> mpSystemRenderEntities;
	std::shared_ptr<ecs::system::UpdateCameraPerspective> mpSystemUpdateCameraPerspective;
	std::shared_ptr<ecs::system::MovePlayerByInput> mpSystemMovePlayerByInput;
	std::shared_ptr<graphics::TextureRegistry> mpTextureRegistry;
	
	std::shared_ptr<ui::DebugHUD> mpDebugHUD;
	std::shared_ptr<ui::TextLogMenu> mpMenuTextLog;

	void registerCommands();
	void exec_setResolution(command::Signature const& cmd);
	void exec_setResolutionRatio(command::Signature const& cmd);
	void exec_setDPI(command::Signature const& cmd);
	void exec_printAccountList(command::Signature const& cmd);
	void exec_createAccount(command::Signature const& cmd);
	void exec_setAccount(command::Signature const& cmd);
	void exec_printAccount(command::Signature const& cmd);
	void exec_setAccountName(command::Signature const& cmd);
	void exec_openSave(command::Signature const& cmd);
	void exec_closeSave(command::Signature const& cmd);
	void exec_joinServer(command::Signature const& cmd);
	void exec_joinServerLocal(command::Signature const& cmd);
	void exec_leaveServer(command::Signature const& cmd);
	void exec_startHostingServer(command::Signature const& cmd);
	void exec_stopHostingServer(command::Signature const& cmd);
	void exec_printConnectedUsers(command::Signature const& cmd);

	void onNetIdReceived(network::Interface *pInterface, ui32 netId);
	void onDedicatedClientAuthenticated(network::Interface *pInterface);
	void onClientPeerStatusChanged(network::Interface *pInterface, ui32 netId, network::EClientStatus status);
	void onDedicatedClientDisconnected(network::Interface *pInterface, ui32 invalidNetId);
	void onNetworkStopped(network::Interface *pInterface);

	void onEVCSOwnershipChanged(ecs::EType ecsType, ecs::TypeId typeId, ecs::IEVCSObject *pObject);
	void addPlayerDisplayParts(std::shared_ptr<ecs::Entity> pEntity);
	void addPlayerControlParts(std::shared_ptr<ecs::Entity> pEntity);

	bool initializeGraphics();

	bool createWindow();
	void destroyWindow();

	void createRenderers();
	void createGameRenderer();
	void loadVoxelTypeTextures();
	void createPipelineRenderers();
	void createVoxelGridRenderer();
	void createWorldAxesRenderer();
	void createChunkBoundaryRenderer();
	void createUIRenderer();
	void destroyRenderers();

	bool scanResourcePacks();

	void updateWorldGraphics();

};

NS_END
