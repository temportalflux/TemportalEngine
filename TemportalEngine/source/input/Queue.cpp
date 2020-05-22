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

ListenerMap::iterator dereferenceListenerHandle(ListenerHandle &handle)
{
	return *((ListenerMap::iterator*)handle);
}

ListenerHandle Queue::addListener(EInputType evt, Listener listener)
{
	ListenerMap::iterator handle = this->mListenersByEvent.insert(std::make_pair(evt, listener));
	return std::addressof(handle);
}

void Queue::removeListener(ListenerHandle &handle)
{
	this->mListenersByEvent.erase(dereferenceListenerHandle(handle));
	handle = nullptr;
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

	// Iterate over all iterators whose key matches the event type
	std::pair<ListenerMap::iterator, ListenerMap::iterator> listeners = this->mListenersByEvent.equal_range(evt.type);
	for (auto iter = listeners.first; iter != listeners.second; ++iter)
	{
		// Execute each listener
		(iter->second)(evt);
	}
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
