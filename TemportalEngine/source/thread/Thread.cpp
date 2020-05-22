#include "thread/Thread.hpp"
#include <thread>

Thread::Thread()
{
	mpThreadHandle = nullptr;
}

Thread::Thread(char const * name, logging::LogSystem * pLogSystem)
	: mpName(name)
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

void Thread::start(std::function<bool()> functor)
{
	mpThreadHandle = new std::thread(std::bind(&Thread::run, this, functor));
}

void Thread::run(std::function<bool()> functor)
{
	mLogger.log(logging::ECategory::LOGINFO, "Starting thread %s", mpName);
	while (functor());
	mLogger.log(logging::ECategory::LOGINFO, "Stopping thread %s", mpName);
}

void Thread::join()
{
	std::thread *pThread = (std::thread *)this->mpThreadHandle;
	pThread->join();

	delete pThread;
	this->mpThreadHandle = nullptr;
}
