#ifndef INPUT_QUEUE_HPP
#define INPUT_QUEUE_HPP

#include "Namespace.h"
#include "types/integer.h"
#include "Event.hpp"
#include "Mutex.hpp"

NS_INPUT

// http://gameprogrammingpatterns.com/event-queue.html
class Queue
{
public:
	typedef void(*DelegateListener)(Event const &evt);

private:
	typedef ui8 TMaxSize;
	static const TMaxSize MAX_COUNT_PENDING = 255;
	
	MutexLock mpMutex[1];
	Event mpBuffer[MAX_COUNT_PENDING];
	TMaxSize mIndexHead, mIndexTail;

	DelegateListener mpListener;

	Event const dequeueRaw();
	bool enqueueRaw(Event const &evt);
	void dispatchRaw();

public:
	Queue(DelegateListener listener);
	Queue();

	inline bool hasPending() const;
	inline bool enqueue(Event const& evt);
	inline void dispatch();
	inline void dispatchAll();

};

NS_END

#endif // INPUT_QUEUE_HPP
