#include "logging/Logger.hpp"

// Libraries ------------------------------------------------------------------
#include <cassert>
#include <fstream>
#include <stdarg.h>
#include <string>
#include <time.h>

// Engine ---------------------------------------------------------------------
#include "math/compare.h"

// Namespaces -----------------------------------------------------------------
using namespace logging;

// Log System -----------------------------------------------------------------

LogSystem::LogSystem()
{
	mpFileStream = nullptr;
	*mpLock = thread::MutexLock();
}

void LogSystem::printLog(char const *const format, char * args)
{
	vfprintf(stdout, format, args);
	if (mpFileStream != 0)
	{
		vfprintf((FILE*)mpFileStream, format, args);
	}
}

void LogSystem::printLog(char const *const format, ...)
{
	va_list args;
	va_start(args, format);
	printLog(format, args);
	va_end(args);
}

void LogSystem::open(char const * const filePath)
{
	fopen_s((FILE**)&mpFileStream, filePath, "w");
}

void LogSystem::log(Logger *pLogger, ECategory category, Message format, ...)
{
	const char* categoryStr;
	switch (category)
	{
	case ECategory::LOGINFO:
		categoryStr = "INFO";
		break;
	case ECategory::LOGWARN:
		categoryStr = "WARNING";
		break;
	case ECategory::LOGERROR:
		categoryStr = "ERROR";
		break;
	case ECategory::LOGDEBUG:
		categoryStr = "DEBUG";
		break;
	default:
		categoryStr = "";
		break;
	}

	time_t currentTime = time(nullptr);
	struct tm timeinfo;
	localtime_s(&timeinfo, &currentTime);
	char timeStr[70];
	strftime(timeStr, sizeof(timeStr), "%Y.%m.%d %H:%M:%S", &timeinfo);

	va_list args;
	va_start(args, format);

	mpLock->lock();
	printLog("[%s][%s] %s> ", timeStr, categoryStr, pLogger->mpTitle);
	printLog(format, args);
	printLog("\n");
	mpLock->unlock();

	va_end(args);
}

bool LogSystem::close()
{
	assert(mpFileStream != 0);
	return fclose((FILE*)mpFileStream) == 0;
}

// Logger ---------------------------------------------------------------------

Logger::Logger(char const * title, LogSystem *pLogSystem)
	: mpLogSystem(pLogSystem)
{
	strncpy_s(mpTitle, title, minimum(strlen(title), LOGGER_MAX_TITLE_LENGTH));
}

Logger::Logger() : Logger("", 0)
{
}
