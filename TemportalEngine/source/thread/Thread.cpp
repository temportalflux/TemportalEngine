#include "thread/Thread.hpp"
#include <thread>

void Thread::updateInternal(void * params)
{
	mLogger.log(logging::ECategory::LOGINFO, "Starting thread");
	(*mpDelegateUpdate)(params);
	mLogger.log(logging::ECategory::LOGINFO, "Stopping thread");
}

void Thread::updateInternalStatic(Thread * pThread, void * params)
{
	pThread->updateInternal(params);
}

Thread::Thread()
{
	mpThreadHandle = nullptr;
}

Thread::Thread(char const * name, logging::LogSystem * pLogSystem, DelegateUpdate update)
	: mpName(name)
	, mpDelegateUpdate(update)
{
	mpThreadHandle = nullptr;
	mLogger = logging::Logger(mpName, pLogSystem);
}

Thread::~Thread()
{
	if (this->mpThreadHandle != nullptr)
	{
		this->join();
	}
}

void Thread::start(void * params)
{
	mpThreadHandle = new std::thread(&updateInternalStatic, this, params);
}

void Thread::join()
{
	std::thread *pThread = (std::thread *)this->mpThreadHandle;
	pThread->join();

	delete pThread;
	this->mpThreadHandle = nullptr;
}
