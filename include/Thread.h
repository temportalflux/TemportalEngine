#ifndef INCLUDE_THREAD_H_
#define INCLUDE_THREAD_H_

#include <thread>

template <typename... TArgs>
class Thread
{
public:
	typedef void (*DelegateUpdate)(TArgs...);

private:
	char const* mpName;
	//std::thread mpHandle[1];

public:
	Thread() {}
	Thread(char const* name) : mpName(name)
	{
	}

	void start(DelegateUpdate update, TArgs... args)
	{
		//*mpHandle = std::thread(update, args...);
	}

	void join()
	{
		//this->mpHandle->join();
	}

};

#endif  // INCLUDE_THREAD_H_
