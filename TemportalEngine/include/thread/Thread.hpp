#ifndef INCLUDE_THREAD_H_
#define INCLUDE_THREAD_H_

#include "TemportalEnginePCH.hpp"

#include <functional>
#include <optional>

#include "logging/Logger.hpp"

class Thread
{
public:
	typedef std::function<bool()> DelegateRun;
	typedef std::optional<std::function<void()>> DelegateOnComplete;

public:
	Thread();
	Thread(std::string name, logging::LogSystem *pLogSystem);
	~Thread();

	void setFunctor(DelegateRun functor);
	void setOnComplete(DelegateOnComplete onComplete);

	bool isValid() const;

	/**
		Starts the thread and runs the callable `functor`.
		While `functor` returns true, the thread will continue.
		If `functor` returns false, the thread will end.
	*/
	void start();
	void join();

private:
	std::string mpName;
	logging::Logger mLogger;

	DelegateRun mFunctorDelegate;
	DelegateOnComplete mOnCompleteDelegate;

	void* mpThreadHandle;

	void run();

};

#endif  // INCLUDE_THREAD_H_
