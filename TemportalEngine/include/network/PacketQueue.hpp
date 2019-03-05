#ifndef NETWORK_PACKETQUEUE_HPP
#define NETWORK_PACKETQUEUE_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers

#include "network/Packet.hpp"
#include "types/integer.h"
#include "thread/MutexLock.hpp"

NS_NETWORK

// A class to handle a queue of Packets (wrapped RakNet packets).
// Operates as a true queue (sub-structure of LinkedList).
class TEMPORTALENGINE_API PacketQueue
{

private:
	thread::MutexLock mpMutex[1];
	Packet mpRoundQueue[MAX_PACKET_COUNT];

	uSize mIndexHead;
	uSize mIndexTail;

public:
	PacketQueue();
	~PacketQueue();

	// Pushes a packet onto the end of the list
	bool enqueue(Packet const packet);

	// Pops a packet from the front of the list
	// Parameter will be set with the first packet
	bool dequeue(Packet &packet);

	// Returns true if there are no packets in the queue (count == 0)
	bool isEmpty();

	bool const canEnqueue() const;

};

NS_END

#endif