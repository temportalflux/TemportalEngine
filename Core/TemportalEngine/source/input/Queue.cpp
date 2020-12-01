#include "input/Queue.hpp"
#include <string.h> // memset

using namespace input;

Queue::Queue()
{
	*mpMutex = thread::MutexLock();
	memset(mpBuffer, 0, MAX_COUNT_PENDING * sizeof(Event));
	mIndexHead = 0;
	mIndexTail = 0;
}

bool Queue::hasPending() const
{
	return mIndexHead != mIndexTail;
}

Event const Queue::dequeueRaw()
{
	Event const evt = mpBuffer[mIndexHead];
	memset(mpBuffer + mIndexHead, 0, sizeof(Event));
	mIndexHead = (mIndexHead + 1) % MAX_COUNT_PENDING;
	return evt;
}

bool Queue::enqueueRaw(Event const &evt)
{
	if ((mIndexTail + 1) % MAX_COUNT_PENDING == mIndexHead) return false;
	// Add to the end of the list.
	memcpy_s(mpBuffer + mIndexTail, sizeof(Event), &evt, sizeof(Event));
	mIndexTail = (mIndexTail + 1) % MAX_COUNT_PENDING;
	return true;
}

bool Queue::enqueue(Event const& evt)
{
	this->mpMutex->lock();
	bool ret = this->enqueueRaw(evt);
	this->mpMutex->unlock();
	return ret;
}

void Queue::dispatchRaw()
{
	// If there are no pending requests, do nothing.
	if (mIndexHead == mIndexTail) return;
	Event const& evt = this->dequeueRaw();
	this->OnInputEvent.broadcast(evt.type, evt);
}

void Queue::dispatch()
{
	// If there are no pending requests, do nothing.
	if (mIndexHead == mIndexTail) return;
	mpMutex->lock();
	this->dispatchRaw();
	mpMutex->unlock();
}

void Queue::dispatchAll()
{
	while (this->hasPending())
		this->dispatch();
}
