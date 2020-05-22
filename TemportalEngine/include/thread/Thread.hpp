#ifndef INCLUDE_THREAD_H_
#define INCLUDE_THREAD_H_

#include "TemportalEnginePCH.hpp"

#include <functional>

#include "logging/Logger.hpp"

class Thread
{
public:
	typedef void (*DelegateUpdate)(void*);

private:
	char const* mpName;
	logging::Logger mLogger;
	void* mpThreadHandle;

	void run(std::function<bool()> functor);

public:
	Thread();
	Thread(char const* name, logging::LogSystem *pLogSystem);
	~Thread();

	/**
		Starts the thread and runs the callable `functor`.
		While `functor` returns true, the thread will continue.
		If `functor` returns false, the thread will end.
	*/
	void start(std::function<bool()> functor);
	void join();

};

#endif  // INCLUDE_THREAD_H_
