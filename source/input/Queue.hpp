#ifndef INPUT_QUEUE_HPP
#define INPUT_QUEUE_HPP

#include "Namespace.h"
#include "Event.hpp"
#include "types/integer.h"

NS_INPUT

// http://gameprogrammingpatterns.com/event-queue.html
class Queue
{
private:
	typedef types::ui8 TMaxSize;
	static const TMaxSize MAX_PENDING = 255;
	
	Event buffer[MAX_PENDING];
	TMaxSize mIndexHead, mIndexTail;

};

NS_END

#endif // INPUT_QUEUE_HPP
