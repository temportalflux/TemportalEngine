#ifndef NETWORK_EVENT_HPP
#define NETWORK_EVENT_HPP

#include "Namespace.h"
#include "Api.h"
#include "types/integer.h"
#include <functional>

NS_NETWORK

typedef ui32 EventTypeId;

template <typename TData>
class TEMPORTALENGINE_API Event
{
public:
	//typedef std::function<void(TData /*data*/)> Callback;

private:
	EventTypeId mId;
	TData mData;
	//std::function<void(TData /*data*/)> mCallback;

public:
	Event(EventTypeId id)//, Callback callback)
		: mId(id)//, mCallback(callback)
	{ }

	void operator()() const
	{
		//mCallback(mData);
	}

};

NS_END

#endif
