#pragma once

#include "game/GameSession.hpp"

#include "game/GameUserIdRegistry.hpp"
#include "input/Event.hpp"
#include "settings/ClientSettings.hpp"
#include "ui/Core.hpp"
#include "world/WorldCoordinate.hpp"

class Window;
NS_EVCS
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
FORWARD_DEF(NS_GRAPHICS, class VoxelGridRenderer);
FORWARD_DEF(NS_RESOURCE, class PackManager);
FORWARD_DEF(NS_SAVE_DATA, class Instance);
FORWARD_DEF(NS_UI, class FontOwner);
FORWARD_DEF(NS_UI, class DebugHUD);
FORWARD_DEF(NS_UI, class TextLogMenu);
FORWARD_DEF(NS_UI, class WidgetRenderer);
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
	std::shared_ptr<ui::WidgetRenderer> widgetRenderer();
	std::shared_ptr<ui::TextLogMenu> chat();
	game::ClientSettings& settings();

	logging::Logger& logger() override;
	void init() override;
	void uninit() override;

	void setupNetwork(network::Address const& serverAddress);

	utility::Guid const& localUserId() const;
	crypto::RSAKey localUserAuthKey() const;
	network::Identifier localUserNetId() const;
	bool hasLocalEntity() const;
	evcs::Identifier localUserEntityId() const;
	void setLocalUserNetId(network::Identifier netId);

	game::UserInfo& getConnectedUserInfo(network::Identifier netId);

protected:
	void addConnectedUser(network::Identifier netId) override;
	void removeConnectedUser(network::Identifier netId) override;

private:
	game::ClientSettings mClientSettings;

	std::optional<utility::Guid> mLocalUserId;
	bool hasAccount() const;
	game::UserInfo localUserInfo() const;
	std::optional<ui32> mLocalUserNetId;
	std::map<ui32, game::UserInfo> mConnectedUserInfo;
	std::optional<evcs::Identifier> mLocalPlayerEntityId;
	world::Coordinate mPrevLocalEntityPosition;

	std::shared_ptr<resource::PackManager> mpResourcePackManager;

	std::shared_ptr<Window> mpWindow;
	std::shared_ptr<graphics::ImmediateRenderSystem> mpRenderer;

	std::shared_ptr<game::VoxelModelManager> mpVoxelModelManager;
	std::shared_ptr<world::BlockInstanceBuffer> mpVoxelInstanceBuffer;
	
	std::shared_ptr<graphics::VoxelGridRenderer> mpVoxelGridRenderer;
	std::shared_ptr<graphics::SimpleLineRenderer> mpWorldAxesRenderer;
	std::shared_ptr<graphics::ChunkBoundaryRenderer> mpChunkBoundaryRenderer;
	std::shared_ptr<ui::WidgetRenderer> mpUIRenderer;
	
	std::shared_ptr<graphics::SkinnedModelManager> mpSkinnedModelManager;
	std::shared_ptr<graphics::EntityInstanceBuffer> mpEntityInstanceBuffer;
	std::shared_ptr<evcs::system::RenderEntities> mpSystemRenderEntities;
	std::shared_ptr<evcs::system::UpdateCameraPerspective> mpSystemUpdateCameraPerspective;
	std::shared_ptr<evcs::system::MovePlayerByInput> mpSystemMovePlayerByInput;
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

	void sendAuthenticationId(network::Interface *pNetwork, network::ConnectionId connectionId);
	void onClientAuthenticated(network::Interface *pInterface, network::Identifier netId);
	void onClientPeerStatusChanged(network::Interface *pInterface, network::Identifier netId, network::EClientStatus status);
	void onDedicatedClientDisconnected(network::Interface *pInterface, network::Identifier invalidNetId);
	void onNetworkStopped(network::Interface *pInterface);

	void onEVCSOwnershipChanged(evcs::EType ecsType, evcs::TypeId typeId, evcs::IEVCSObject *pObject);
	void addPlayerDisplayParts(std::shared_ptr<evcs::Entity> pEntity);
	void addPlayerControlParts(std::shared_ptr<evcs::Entity> pEntity);

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

	void bindInput();
	void unbindInput();
	void onInputKey(input::Event const& evt);

	void onWorldSimulate(f32 const& deltaTime);

};

NS_END
