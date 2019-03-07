#ifndef TE_NETWORK_SERVICE_HPP
#define TE_NETWORK_SERVICE_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "network/common/NetworkInterface.hpp"
#include "network/common/PacketRegistry.hpp"
#include "thread/Thread.hpp"

NS_ENGINE
class Engine;
NS_END

NS_NETWORK

class TEMPORTALENGINE_API Service
{
	friend class engine::Engine;

private:
	static void runThread(void* pInterface);

	PacketRegistry mpPacketExecutorRegistry[1];

	Thread *mpThread;

	void startThread(engine::Engine *const pEngine);
	void joinThread();

	void processAllPackets();
	void processPacket(PacketInternal const &packet);

protected:
	NetworkInterface mpNetworkInterface[1];

	Service();

	void registerPackets();
	void registerServerPackets();
	void registerClientPackets();
	void registerCommonPackets();

public:

	// TODO: move to cpp
	bool registerPacket(RegistryIdentifier &id, std::function<Packet*(void*)> packet)
	{
		return mpPacketExecutorRegistry->registerType(id, packet);
	}

	bool const isActive() const;

	bool const isServer() const;

	void disconnect();
	
};

NS_END

#endif