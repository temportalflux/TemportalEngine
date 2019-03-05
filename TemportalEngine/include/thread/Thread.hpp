#ifndef INCLUDE_THREAD_H_
#define INCLUDE_THREAD_H_

#include "TemportalEnginePCH.hpp"

// TODO: Namespace
// TODO: Organize Headers

#include "logging/Logger.hpp"

class TEMPORTALENGINE_API Thread
{
public:
	typedef void (*DelegateUpdate)(void*);

private:
	char const* mpName;
	logging::Logger mLogger;
	DelegateUpdate mpDelegateUpdate;
	void* mpThreadHandle;

	void updateInternal(void* params);

	static void updateInternalStatic(Thread *pThread, void* params);

public:
	Thread();
	Thread(char const* name, logging::LogSystem *pLogSystem, DelegateUpdate update);
	~Thread();

	void start(void* params);

	void join();

};

#endif  // INCLUDE_THREAD_H_
