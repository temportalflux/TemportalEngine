#ifndef TE_LOGGING_LOGGER_HPP
#define TE_LOGGING_LOGGER_HPP

#include "Api.h"

#include "Namespace.h"
#include "thread/MutexLock.hpp"

NS_LOGGING

enum class TEMPORTALENGINE_API ECategory
{
	INVALID = -1,

	DEBUG,
	INFO,
	WARN,
	ERROR,

	COUNT
};

typedef char const * Message;

// handles logic and locking of console and file output
class TEMPORTALENGINE_API LogSystem
{
	TE_MutexLock mpLock[1];
	void* mpFileStream;

	void printLog(char const *const format, char *args);
	void printLog(char const *const format, ...);
	void printPrefix(class Logger *pLogger, ECategory category);

public:
	LogSystem();
	void open(char const *const filePath);
	void log(Logger *pLogger, ECategory category, Message format, ...);
	bool close();

};

// Other folks can log from me
class TEMPORTALENGINE_API Logger
{
	friend class LogSystem;

	char mpTitle[255];
	LogSystem *mpLogSystem;

public:
	Logger(char const * title, LogSystem *pLogSystem);
	Logger();

	template <typename... TArgs>
	void log(ECategory category, Message format, TArgs... args)
	{
		mpLogSystem->log(this, category, format, args...);
	}

};

template <typename... TArgs>
void log(char const *title, LogSystem *pLogSystem, ECategory category, Message format, TArgs... args)
{
	Logger(title, pLogSystem).log(category, format, args...);
}

NS_END

#endif