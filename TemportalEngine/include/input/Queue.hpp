#ifndef TE_INPUT_QUEUE_HPP
#define TE_INPUT_QUEUE_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers

#include "types/integer.h"
#include "Event.hpp"
#include "thread/MutexLock.hpp"

NS_INPUT

// http://gameprogrammingpatterns.com/event-queue.html
class TEMPORTALENGINE_API Queue
{
public:
	typedef void(*DelegateListener)(Event const &evt);

private:
	typedef ui8 TMaxSize;
	static const TMaxSize MAX_COUNT_PENDING = 255;
	
	thread::MutexLock mpMutex[1];
	Event mpBuffer[MAX_COUNT_PENDING];
	TMaxSize mIndexHead, mIndexTail;

	DelegateListener mpListener;

	Event const dequeueRaw();
	bool enqueueRaw(Event const &evt);
	void dispatchRaw();

public:
	Queue(DelegateListener listener);
	Queue();

	bool hasPending() const;
	bool enqueue(Event const& evt);
	void dispatch();
	void dispatchAll();

};

NS_END

#endif // TE_INPUT_QUEUE_HPP
