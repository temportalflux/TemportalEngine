#include "network/common/Service.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"
#include "network/PacketType.hpp"
#include "network/PacketInternal.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

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
	PacketInternal packet;
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

void Service::processPacket(PacketInternal const &packet)
{
	void* pData = (void*)packet.mData.data;
	auto packetId = this->mpNetworkInterface->retrievePacketId(pData);
	if (packetId.has_value())
	{
		auto executor = mpPacketExecutorRegistry->getPacketFrom(packetId, pData);
		if (executor.has_value())
		{
			Packet* packet = executor.value();
			packet->execute();
			// TODO: Use memory manager
			delete packet;
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
