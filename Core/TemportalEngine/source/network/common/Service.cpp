#include "network/common/Service.hpp"

// Engine ---------------------------------------------------------------------
#include "Engine.hpp"
#include "network/PacketType.hpp"
#include "network/PacketInternal.hpp"
#include "network/client/PacketConnectionAccepted.hpp"
#include "network/client/PacketConnectionRejected.hpp"
#include "network/server/PacketNewIncomingConnection.hpp"

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

Service::Service()
{
	mpThread = nullptr;
}

void Service::registerPackets()
{
	this->registerServerPackets();
	this->registerClientPackets();
	this->registerCommonPackets();
}

void Service::registerServerPackets()
{
	if (!registerPacket(PacketNewIncomingConnection::Identification, PacketNewIncomingConnection()))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet server::NewIncomingConnection");
	}
}

void Service::registerClientPackets()
{

	if (!registerPacket(PacketConnectionAccepted::Identification, PacketConnectionAccepted()))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet client::ConnectionRequestAccepted");
	}

	if (!registerPacket(PacketConnectionRejected::Identification, PacketConnectionRejected()))
	{
		LogEngine(logging::ECategory::LOGERROR, "Could not register packet client::ConnectionRequestRejected");
	}

}

void Service::registerCommonPackets()
{
}

void Service::startThread(engine::Engine * const pEngine)
{
	// TODO: this isn't dealloced
	//mpThread = pEngine->alloc<Thread>("Thread-Network", &engine::Engine::LOG_SYSTEM);
	//mpThread->setFunctor(std::bind(&Service::runThread, this));
	//mpThread->start();
}

bool Service::runThread()
{
	engine::Engine *pEngine = nullptr;
	bool isActive = false;// engine::Engine::GetChecked(pEngine) && pEngine->isActive();
	if (isActive)
	{
		mpNetworkInterface->fetchAllPackets();
		//pEngine->getNetworkService().value()->processAllPackets();
	}
	return isActive;
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

bool Service::registerPacket(RegistryIdentifier &id, std::function<Packet*(void*)> packet)
{
	return mpPacketExecutorRegistry->registerType(id, packet);
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
