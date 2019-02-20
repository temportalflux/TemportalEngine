#include "Queue.hpp"
#include <string.h> // memset

using namespace input;

Queue::Queue(DelegateListener listener)
	: mpListener(listener)
{
	*mpMutex = MutexLock();
	memset(mpBuffer, 0, MAX_COUNT_PENDING * sizeof(Event));
	mIndexHead = 0;
	mIndexTail = 0;
}

Queue::Queue() : Queue(nullptr)
{
}

inline bool Queue::hasPending() const
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
	if ((mIndexTail + 1) % MAX_COUNT_PENDING != mIndexHead) return false;
	// Add to the end of the list.
	memcpy_s(mpBuffer + mIndexTail, sizeof(Event), &evt, sizeof(Event));
	mIndexTail = (mIndexTail + 1) % MAX_COUNT_PENDING;
	return true;
}

inline bool Queue::enqueue(Event const& evt)
{
	this->mpMutex->lock();
	bool ret = this->enqueue(evt);
	this->mpMutex->unlock();
	return ret;
}

void Queue::dispatchRaw()
{
	// If there are no pending requests, do nothing.
	if (mIndexHead == mIndexTail) return;
	Event const& evt = this->dequeueRaw();
	if (this->mpListener != nullptr)
	{
		(*this->mpListener)(evt);
	}
}

inline void Queue::dispatch()
{
	// If there are no pending requests, do nothing.
	if (mIndexHead == mIndexTail) return;
	mpMutex->lock();
	this->dispatchRaw();
	mpMutex->unlock();
}

inline void Queue::dispatchAll()
{
	while (this->hasPending())
		this->dispatch();
}
