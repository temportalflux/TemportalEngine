#ifndef TE_NETWORK_PACKETQUEUE_HPP
#define TE_NETWORK_PACKETQUEUE_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "network/PacketInternal.hpp"
#include "thread/MutexLock.hpp"
#include "types/integer.h"

// ----------------------------------------------------------------------------
NS_NETWORK

// A class to handle a queue of Packets (wrapped RakNet packets).
// Operates as a true queue (sub-structure of LinkedList).
class TEMPORTALENGINE_API PacketQueue
{

private:
	thread::MutexLock mpMutex[1];
	PacketInternal mpRoundQueue[MAX_PACKET_COUNT];

	uSize mIndexHead;
	uSize mIndexTail;

public:
	PacketQueue();
	~PacketQueue();

	// Pushes a packet onto the end of the list
	bool enqueue(PacketInternal const packet);

	// Pops a packet from the front of the list
	// Parameter will be set with the first packet
	bool dequeue(PacketInternal &packet);

	// Returns true if there are no packets in the queue (count == 0)
	bool isEmpty();

	bool const canEnqueue() const;

};

NS_END
// ----------------------------------------------------------------------------

#endif