#ifndef INCLUDE_THREAD_H_
#define INCLUDE_THREAD_H_

#include <thread>
#include "Log.h"

template <typename... TArgs>
class Thread
{
public:
	typedef void (*DelegateUpdate)(TArgs...);

private:
	char const* mpName;
	std::thread mpHandle[1];
	DelegateUpdate mpDelegateUpdate;

	void updateInternal(TArgs... args)
	{
		logging::log(mpName, logging::ECategory::INFO, "Starting thread");
		(*mpDelegateUpdate)(args...);
		logging::log(mpName, logging::ECategory::INFO, "Stopping thread");
	}

	static void updateInternalStatic(Thread<TArgs...> *pThread, TArgs... args)
	{
		pThread->updateInternal(args...);
	}

public:
	Thread() {}
	Thread(char const* name, DelegateUpdate update)
		: mpName(name)
		, mpDelegateUpdate(update)
	{
	}

	void start(TArgs... args)
	{
		*mpHandle = std::thread(&updateInternalStatic, this, args...);
	}

	void join()
	{
		this->mpHandle->join();
	}

};

#endif  // INCLUDE_THREAD_H_
