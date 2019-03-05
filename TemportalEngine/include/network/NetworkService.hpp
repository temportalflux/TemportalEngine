#ifndef NETWORK_SERVICE_HPP
#define NETWORK_SERVICE_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers

#include "network/NetworkInterface.hpp"
#include "thread/Thread.hpp"
#include "network/PacketExecutorRegistry.hpp"

NS_ENGINE
class Engine;
NS_END

NS_NETWORK

class TEMPORTALENGINE_API Service
{
	friend class engine::Engine;

private:
	static void runThread(void* pInterface);

	PacketExecutorRegistry mpPacketExecutorRegistry[1];

	Thread *mpThread;

	void startThread(engine::Engine *const pEngine);
	void joinThread();

	void processAllPackets();
	void processPacket(Packet const &packet);

protected:
	NetworkInterface mpNetworkInterface[1];

	Service();

public:

	std::optional<Packet::Id> registerPacket(PacketExecutorRegistry::DelegatePacketExecutor callback);
	bool registerPacket(Packet::Id &idOut, PacketExecutorRegistry::DelegatePacketExecutor callback);

	bool const isActive() const;

	bool const isServer() const;

	void disconnect();
	
};

NS_END

#endif