#include "network/NetworkInterface.hpp"
#include "Engine.hpp"

#include "network/RakNet.hpp"

const uSize SIZE_OF_TIMESTAMPS = sizeof(char) + sizeof(RakNet::Time) + sizeof(RakNet::Time);

using namespace network;

typedef RakNet::RakPeerInterface * Interface;
#define GetInterface(ptr) static_cast<Interface>(ptr)

void NetworkInterface::runThread(void* pInterface)
{
	engine::Engine *pEngine = nullptr;
	while (engine::Engine::GetChecked(pEngine) && pEngine->isActive())
	{
		((NetworkInterface *)pInterface)->fetchPacket();
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
void NetworkInterface::initServer(const ui16 port, const ui16 maxClients)
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
void NetworkInterface::connectToServer(char const *address, const ui16 port)
{
	// Connect to the server using the specified address and port
	GetInterface(mpPeerInterface)->Connect(address, port, 0, 0);
}

// Fetch the address the peer is bound to
void NetworkInterface::queryAddress() const
{
	auto address = GetInterface(mpPeerInterface)->GetMyBoundAddress();
	//address.address;
}

// Return the IP string from the peer
char const * NetworkInterface::getIP() const
{
	return GetInterface(mpPeerInterface)->GetLocalIP(0); // note: this can be finicky if there are multiple network addapters
}

// Returns true if the network interface (RakNet thread) is active
bool NetworkInterface::isActive() const
{
	return GetInterface(mpPeerInterface)->IsActive();
}

i32 readTimestamps(const ui8 *buffer, ui64 &time1, ui64 &time2)
{
	if (buffer)
	{
		i8 tag;
		tag = *(buffer++);
		if (tag == (i8)ID_TIMESTAMP)
		{
			const ui64 *tPtr = (RakNet::Time *)buffer;
			time1 = *(tPtr++);
			time2 = *(tPtr++);
			if (*(buffer + 4) < 0)
				// RakNet seems to be subtracting this number for some stupid reason... and only half the time... what is it doing (Dan Buckstein)
				time1 += 4311744512;
			return SIZE_OF_TIMESTAMPS;
		}
	}
	return 0;
}

i32 writeTimestamps(ui8 *buffer, const ui64 &time1, const ui64 &time2)
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

void NetworkInterface::fetchPacket()
{
	Interface pInterface = GetInterface(mpPeerInterface);

	RakNet::Packet *pRakPak = pInterface->Receive();

	// No packet in buffer
	if (pRakPak == nullptr) return;
	
	// Copy out the addresss
	Packet::DataPtr address = {};
	auto addressData = pRakPak->systemAddress.ToString();
	address.length = std::strlen(addressData);
	assert(address.length <= MAX_PACKET_ADDRESS_LENGTH);
	memcpy_s(address.data, address.length * sizeof(i8), pRakPak->systemAddress.ToString(), sizeof(address.length));

	const i32 lastPing = pInterface->GetLastPing(pRakPak->systemAddress);

	// sentTime_local - The local time that the packet was sent at
	// sentTime_remote - The remote time the packet was sent at
	// sentToReadDiff_local - The local time difference between when the packet was sent and when it was read
	// sentToRead_remote - The local time spent to read the packet (when packet had a pitstop on server)
	// sendToRead_other - The remote time the packet was sent at originally
	RakNet::Time sentTime_local, sentTime_remote, sentToReadDiff_local, sentToRead_remote, sendToRead_other;

	// Time in local clock that the message was read
	RakNet::Time readTime_local = RakNet::GetTime();

	Packet::TimestampInfo timestampInfo;
	timestampInfo.timesLoaded = false;

	uSize sizeOfTimestamps = 0;

	// Try to read off the sent times
	int sizeReadSent = readTimestamps(pRakPak->data + sizeOfTimestamps, sentTime_local, sentTime_remote);

	// sizeRead > 0 when there are timestamps to read
	if (sizeReadSent > 0)
	{
		sentToReadDiff_local = (readTime_local - sentTime_local);
		// compensate for timestamps by removing the size
		sizeOfTimestamps += sizeReadSent;

		int sizeReadAlt = readTimestamps(pRakPak->data + sizeOfTimestamps, sentToRead_remote, sendToRead_other);
		if (sizeReadAlt > 0)
		{
			// compensate for timestamps by removing the size
			sizeOfTimestamps += sizeReadAlt;

			timestampInfo.timesLoaded = true;
			timestampInfo.packetReadTime_local = readTime_local;
			timestampInfo.readDiff_local = sentToReadDiff_local;
			timestampInfo.sentTime_remote = sentToRead_remote;
			timestampInfo.totalTransferTime_local = sentToReadDiff_local + sentToRead_remote;

		}
	}

	// Copy out the packet data
	Packet::DataPacket data;
	data.length = pRakPak->length - sizeOfTimestamps;
	assert(data.length <= MAX_PACKET_DATA_LENGTH);
	memcpy_s(data.data, data.length * sizeof(ui8), pRakPak->data + sizeOfTimestamps, data.length * sizeof(ui8));

	// Send address, and packet data to copy, to a packet wrapper
	auto packet = Packet(address, data);
	packet.mTimestampInfo = timestampInfo;

	// Save packet for processing later
	mpQueue->enqueue(packet);

	pInterface->DeallocatePacket(pRakPak);
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
//*/

/*
// Poll the next cached packet
// Returns true if a packet was found;
bool NetworkInterface::pollPackets(Packet *&nextPacket)
{
	mpPackets->dequeue(nextPacket);
	return nextPacket != NULL;
}
//*/
