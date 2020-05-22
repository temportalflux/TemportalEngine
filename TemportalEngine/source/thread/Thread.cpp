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

void Thread::start(std::function<bool()> functor, std::optional<std::function<void()>> onComplete)
{
	mpThreadHandle = new std::thread(std::bind(&Thread::run, this, functor, onComplete));
}

void Thread::run(std::function<bool()> functor, std::optional<std::function<void()>> onComplete)
{
	mLogger.log(logging::ECategory::LOGINFO, "Starting thread %s", mpName);
	while (functor());
	mLogger.log(logging::ECategory::LOGINFO, "Stopping thread %s", mpName);
	if (onComplete.has_value())
	{
		(onComplete.value())();
	}
}

void Thread::join()
{
	std::thread *pThread = (std::thread *)this->mpThreadHandle;
	pThread->join();

	delete pThread;
	this->mpThreadHandle = nullptr;
}
