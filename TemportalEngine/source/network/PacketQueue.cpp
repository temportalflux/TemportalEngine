#include "network/PacketQueue.hpp"

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <string.h> // memset

using namespace network;

PacketQueue::PacketQueue()
{
	memset(mpRoundQueue, 0, MAX_PACKET_COUNT * sizeof(Packet));
	mIndexHead = 0;
	mIndexTail = 0;
}

PacketQueue::~PacketQueue()
{
}

bool PacketQueue::enqueue(Packet const data)
{
	if ((mIndexTail + 1) % MAX_PACKET_COUNT == mIndexHead) return false;
	mpMutex->lock();
	// Add to the end of the list.
	memcpy_s(mpRoundQueue + mIndexTail, sizeof(Packet), &data, sizeof(Packet));
	mIndexTail = (mIndexTail + 1) % MAX_PACKET_COUNT;
	mpMutex->unlock();
	return true;
}

bool PacketQueue::dequeue(Packet &data)
{
	if ((mIndexTail + 1) % MAX_PACKET_COUNT == mIndexHead) return false;
	mpMutex->lock();
	data = mpRoundQueue[mIndexHead];
	memset(mpRoundQueue + mIndexHead, 0, sizeof(Packet));
	mIndexHead = (mIndexHead + 1) % MAX_PACKET_COUNT;
	mpMutex->unlock();
	return true;
}

// Returns true if there are no packets in the queue (count == 0 / mHead == NULL)
bool PacketQueue::isEmpty()
{
	return mIndexHead == mIndexTail;
}