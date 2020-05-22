#ifndef TE_INPUT_QUEUE_HPP
#define TE_INPUT_QUEUE_HPP

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "input/types.h"
#include "thread/MutexLock.hpp"
#include "Event.hpp"

NS_INPUT

// http://gameprogrammingpatterns.com/event-queue.html
class Queue
{
private:
	typedef ui8 TMaxSize;
	static const TMaxSize MAX_COUNT_PENDING = 255;
	
	thread::MutexLock mpMutex[1];
	Event mpBuffer[MAX_COUNT_PENDING];
	TMaxSize mIndexHead, mIndexTail;

	ListenerMap mListenersByEvent;

	Event const dequeueRaw();
	bool enqueueRaw(Event const &evt);
	void dispatchRaw();

public:
	Queue();

	ListenerHandle addListener(EInputType evt, Listener listener);
	void removeListener(ListenerHandle &handle);

	bool hasPending() const;
	bool enqueue(Event const& evt);
	void dispatch();
	void dispatchAll();

};

NS_END

#endif // TE_INPUT_QUEUE_HPP
