#include "thread/Thread.hpp"
#include <thread>

Thread::Thread()
	: mpThreadHandle(nullptr)
	, mFunctorDelegate([&]() { return false; })
	, mOnCompleteDelegate(std::nullopt)
{
}

Thread::Thread(std::string name, logging::LogSystem * pLogSystem)
	: Thread()
{
	mpName = name;
	mLogger = logging::Logger(mpName.c_str(), pLogSystem);
}

Thread::~Thread()
{
	if (this->mpThreadHandle != nullptr)
	{
		this->join();
	}
}

void Thread::setFunctor(DelegateRun functor)
{
	mFunctorDelegate = functor;
}

void Thread::setOnComplete(DelegateOnComplete onComplete)
{
	mOnCompleteDelegate = onComplete;
}

void Thread::start()
{
	mpThreadHandle = new std::thread(std::bind(&Thread::run, this));
}

void Thread::run()
{
	mLogger.log(logging::ECategory::LOGINFO, "Starting thread %s", mpName.c_str());
	while (mFunctorDelegate());
	mLogger.log(logging::ECategory::LOGINFO, "Stopping thread %s", mpName.c_str());
	if (mOnCompleteDelegate.has_value())
	{
		(mOnCompleteDelegate.value())();
	}
}

void Thread::join()
{
	std::thread *pThread = (std::thread *)this->mpThreadHandle;
	pThread->join();

	delete pThread;
	this->mpThreadHandle = nullptr;
}
