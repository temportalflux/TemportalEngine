#include "network/NetworkService.hpp"
#include "Engine.hpp"
#include "network/PacketType.hpp"
#include "network/Packet.hpp"

using namespace network;

void Service::runThread(void* pInterface)
{
	engine::Engine *pEngine = nullptr;
	while (engine::Engine::GetChecked(pEngine) && pEngine->isActive())
	{
		NetworkInterface *pNetInterface = (NetworkInterface *)pInterface;
		pNetInterface->fetchAllPackets();
		pEngine->getNetworkService().value()->processAllPackets();
	}
}

Service::Service()
{
	mpThread = nullptr;


	if (!registerPacket(network::packets::server::NewIncomingConnection, 0))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet server::NewIncomingConnection");
	}
	if (!registerPacket(network::packets::client::ConnectionRequestAccepted, 0))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet client::ConnectionRequestAccepted");
	}
	if (!registerPacket(network::packets::client::ConnectionRequestRejected, 0))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet client::ConnectionRequestRejected");
	}
}

std::optional<Packet::Id> Service::registerPacket(PacketExecutorRegistry::DelegatePacketExecutor callback)
{
	return this->mpPacketExecutorRegistry->registerPacket(callback);
}

bool Service::registerPacket(Packet::Id & idOut, PacketExecutorRegistry::DelegatePacketExecutor callback)
{
	std::optional<Packet::Id> id = this->registerPacket(callback);
	if (id.has_value())
	{
		idOut = id.value();
		return true;
	}
	return false;
}

void Service::startThread(engine::Engine * const pEngine)
{
	mpThread = pEngine->alloc<Thread>("Thread-Network", &engine::Engine::LOG_SYSTEM, &runThread);
	mpThread->start(mpNetworkInterface);
}

void Service::joinThread()
{
	if (mpThread == nullptr) return;
	mpThread->join();
}

void Service::processAllPackets()
{
	Packet packet;
	while (!this->mpNetworkInterface->mpQueue->isEmpty()
		&& this->mpNetworkInterface->mpQueue->dequeue(packet))
	{
		this->processPacket(packet);
	}
}

template <typename T>
T getData(void* &pBuffer)
{
	T *pData = (T*)pBuffer;
	pBuffer = (void*)(pData + 1);
	return *pData;
}

void Service::processPacket(Packet const &packet)
{
	void* pData = (void*)packet.mData.data;
	auto packetId = this->mpNetworkInterface->retrievePacketId(pData);
	if (packetId.has_value())
	{
		auto executor = mpPacketExecutorRegistry->getPacketExecutor(packetId.value());
		if (executor.has_value())
		{
			(*executor)(packetId.value(), pData);
		}
	}
}

bool const Service::isActive() const
{
	return this->mpNetworkInterface->isActive();
}

bool const Service::isServer() const
{
	return this->mpNetworkInterface->isServer();
}

void Service::disconnect()
{
	this->mpNetworkInterface->disconnect();
}
