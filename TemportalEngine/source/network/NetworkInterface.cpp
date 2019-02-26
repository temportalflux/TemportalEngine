#include "network/NetworkInterface.hpp"
#include "Engine.hpp"

#include "network/RakNet.hpp"

using namespace network;

typedef RakNet::RakPeerInterface * Interface;
#define GetInterface(ptr) static_cast<Interface>(ptr)

void NetworkInterface::runThread(void* pInterface)
{
	engine::Engine *pEngine = nullptr;
	while (engine::Engine::GetChecked(pEngine) && pEngine->isActive())
	{
		NetworkInterface *pNetInterface = (NetworkInterface *)pInterface;
		if (pNetInterface->getPacketCount() > 0)
		{
			//pNetInterface->getNextPacket();
		}
	}
}

NetworkInterface::NetworkInterface()
{
	mpPeerInterface = RakNet::RakPeerInterface::GetInstance();
}

NetworkInterface::~NetworkInterface()
{
	RakNet::RakPeerInterface::DestroyInstance(GetInterface(mpPeerInterface));
	mpPeerInterface = nullptr;
}

// Startup the server interface
void NetworkInterface::initServer(const int port, const int maxClients)
{
	// Startup the server by reserving a port
	RakNet::SocketDescriptor sd = RakNet::SocketDescriptor(port, 0);
	Interface pInterface = GetInterface(mpPeerInterface);
	pInterface->Startup(maxClients, &sd, 1);
	pInterface->SetMaximumIncomingConnections(maxClients);
}

// Startup the client interface
void NetworkInterface::initClient()
{
	// Startup the client by starting on an empty socket
	RakNet::SocketDescriptor sd;
	GetInterface(mpPeerInterface)->Startup(1, &sd, 1);
}

// Connect the interface to its destination
void NetworkInterface::connectToServer(char const *address, const int port)
{
	// Connect to the server using the specified address and port
	GetInterface(mpPeerInterface)->Connect(address, port, 0, 0);
}

// Fetch the address the peer is bound to
void NetworkInterface::queryAddress()
{
	auto address = GetInterface(mpPeerInterface)->GetMyBoundAddress();
	//address.address;
}

// Return the IP string from the peer
char const * NetworkInterface::getIP()
{
	return GetInterface(mpPeerInterface)->GetLocalIP(0); // note: this can be finicky if there are multiple network addapters
}

// Returns true if the network interface (RakNet thread) is active
bool NetworkInterface::isActive()
{
	return GetInterface(mpPeerInterface)->IsActive();
}

int NetworkInterface::getPacketCount() const
{
	//return this->mpPackets->getCount();
	return 0;
}

// Shutdown the peer interface
void NetworkInterface::disconnect()
{
	GetInterface(mpPeerInterface)->Shutdown(500);
}

/*
// Send packet data over RakNet
void Network::sendTo(Data data, DataSize size,
	RakNet::SystemAddress *address,
	PacketPriority *priority, PacketReliability *reliability,
	char channel, bool broadcast, bool timestamp, const TimeStamp *timestampInfo)
{
	char *msg = data;
	int totalSize = size;

	TimeStamp tInfo = {};
	if (timestampInfo != NULL)
	{
		tInfo = *timestampInfo;
	}

	if (timestamp)
	{
		// Always add a timestamp, where the first byte is the ID for timestamps
		// Get the current time
		// Added By Jake
		if (timestampInfo == NULL) tInfo.packetReadTime_local = RakNet::GetTime();
		// Added By Jake
		totalSize = size + SIZE_OF_TIMESTAMPS * 2;

		//std::string s = std::string("Adding timestamps, moving size from ") + std::to_string(size) + " to " + std::to_string(totalSize);
		//this->sendLog(s.c_str(), 0);

		// Create a new message byte[] to  contain the original data ADND the timestamp stuff
		// Added By Jake
		msg = new char[totalSize];
		char *head = msg;
		// Write the local time
		// Added By Jake
		head += this->writeTimestamps(head, tInfo.packetReadTime_local, tInfo.packetReadTime_local);
		// Write empty slots
		// Added By Jake
		head += this->writeTimestamps(head, tInfo.readDiff_local, tInfo.sentTime_remote);
		// Write the remaining bytes
		// Added By Jake
		memcpy(head, data, size);
		head += size;
	}

	this->mpPeerInterface->Send(msg, totalSize, *priority, *reliability, channel, *address, broadcast);
}

// Cache all incoming packets (should be run regularly)
void Network::fetchAllPackets()
{
	// RakNet Packet pointer
	RakNet::Packet *packet;
	// Wrapper Packet pointer
	ChampNet::Packet* pCurrentPacket;

	// Iterate over all packets in the interface
	for (packet = mpPeerInterface->Receive();
		packet;
		// DEALLOCATE PACKET WHEN FINISHED ITERATION
		mpPeerInterface->DeallocatePacket(packet), packet = mpPeerInterface->Receive())
	{
		// Process the packet

		// Copy out the addresss
		const char* address = packet->systemAddress.ToString();
		unsigned int addressLength = (unsigned int)std::strlen(address);

		char *data = (char *)packet->data;
		unsigned int dataSize = packet->length;
		const int lastPing = mpPeerInterface->GetLastPing(packet->systemAddress);

		// sentTime_local - The local time that the packet was sent at
		// sentTime_remote - The remote time the packet was sent at
		// sentToReadDiff_local - The local time difference between when the packet was sent and when it was read
		// sentToRead_remote - The local time spent to read the packet (when packet had a pitstop on server)
		// sendToRead_other - The remote time the packet was sent at originally
		// Added By Jake
		RakNet::Time sentTime_local, sentTime_remote, sentToReadDiff_local, sentToRead_remote, sendToRead_other;

		// Time in local clock that the message was read
		// Added By Jake
		RakNet::Time readTime_local = RakNet::GetTime();

		ChampNet::TimeStamp timestampInfo;
		timestampInfo.timesLoaded = false;

		// Try to read off the sent times
		int sizeReadSent = this->readTimestamps(data, sentTime_local, sentTime_remote);

		// sizeRead > 0 when there are timestamps to read
		if (sizeReadSent > 0)
		{
			// Added By Jake
			sentToReadDiff_local = (readTime_local - sentTime_local);
			// compensate for timestamps by removing the size
			dataSize -= sizeReadSent;

			// Added By Jake
			int sizeReadAlt = this->readTimestamps(data, sentToRead_remote, sendToRead_other);
			if (sizeReadAlt > 0)
			{
				// compensate for timestamps by removing the size
				dataSize -= sizeReadAlt;

				//printf("Read time (local) = %I64d | (last pring = %d) \n", readTime_local, lastPing);
				//printf("Sent time (local) = %I64d | Sent time (remote) = %I64d \n", sentTime_local, sentTime_remote);
				//printf("Sent->Read time diff = %I64d | Clock diff = %I64d \n", sentToReadDiff_local, (sentTime_local - sentTime_remote));
				// Dustin
				timestampInfo.timesLoaded = true;
				timestampInfo.packetReadTime_local = readTime_local;
				timestampInfo.readDiff_local = sentToReadDiff_local;
				timestampInfo.sentTime_remote = sentToRead_remote;
				timestampInfo.totalTransferTime_local = sentToReadDiff_local + sentToRead_remote;

			}
		}

		// Send address, and packet data to copy, to a packet wrapper
		pCurrentPacket = new ChampNet::Packet(addressLength, address, dataSize, packet->data + (packet->length - dataSize));
		pCurrentPacket->timestampInfo = timestampInfo;

		// Save packet for processing later

		mpPackets->enqueue(pCurrentPacket);
	}
}
//*/

// Poll the next cached packet
// Returns true if a packet was found;
/*
bool NetworkInterface::pollPackets(Packet *&nextPacket)
{
	mpPackets->dequeue(nextPacket);
	return nextPacket != NULL;
}

int Network::writeTimestamps(char *buffer, const RakNet::Time &time1, const RakNet::Time &time2)
{
	if (buffer)
	{
		*(buffer++) = (char)(ID_TIMESTAMP);
		RakNet::Time *tPtr = (RakNet::Time *)buffer;
		*(tPtr++) = time1;
		*(tPtr++) = time2;
		return SIZE_OF_TIMESTAMPS;
	}
	return 0;
}

int Network::readTimestamps(const char *buffer, RakNet::Time &time1, RakNet::Time &time2)
{
	if (buffer)
	{
		char tag;
		tag = *(buffer++);
		if (tag == (char)ID_TIMESTAMP)
		{
			const RakNet::Time *tPtr = (RakNet::Time *)buffer;
			time1 = *(tPtr++);
			time2 = *(tPtr++);
			if (*(buffer + 4) < 0)
				time1 += 4311744512;    // RakNet seems to be subtracting this number for some stupid reason... and only half the time... what is it doing (Dan Buckstein)
			return SIZE_OF_TIMESTAMPS;
		}
	}
	return 0;
}
//*/
