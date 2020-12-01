#ifndef TE_INPUT_QUEUE_HPP
#define TE_INPUT_QUEUE_HPP

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "thread/MutexLock.hpp"
#include "Event.hpp"
#include "Delegate.hpp"

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

	Event const dequeueRaw();
	bool enqueueRaw(Event const &evt);
	void dispatchRaw();

public:
	Queue();

	KeyedBroadcastDelegate<EInputType, void(Event const &)> OnInputEvent;
	
	bool hasPending() const;
	bool enqueue(Event const& evt);
	void dispatch();
	void dispatchAll();

};

NS_END

#endif // TE_INPUT_QUEUE_HPP
