#include "logging/Logger.hpp"

// Libraries ------------------------------------------------------------------
#include <cassert>
#include <fstream>
#include <stdarg.h>
#include <array>

// Engine ---------------------------------------------------------------------
#include "math/compare.h"
#include "utility/TimeUtils.hpp"

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

std::string LogSystem::getCategoryShortString(ECategory category)
{
	switch (category)
	{
	case ECategory::LOGINFO: return " INFO";
	case ECategory::LOGWARN: return " WARN";
	case ECategory::LOGERROR: return "ERROR";
	case ECategory::LOGDEBUG: return "DEBUG";
	default: return "?????";
	}
}

void LogSystem::open(char const * const filePath)
{
	fopen_s((FILE**)&mpFileStream, filePath, "w");
}

void LogSystem::log(Logger *pLogger, ECategory category, Message format, ...)
{
	auto categoryStr = getCategoryShortString(category);

	auto const& timeStr = utility::currentTimeStr();

	va_list args;
	va_start(args, format);

	mpLock->lock();
	printLog("[%s][%s] %s> ", timeStr.c_str(), categoryStr.c_str(), pLogger->mpTitle);
	printLog(format, args);
	printLog("\n");
	fflush(stdout);
	mpLock->unlock();

	std::array<char, 256> logContentData;
	logContentData.fill('\0');
	vsnprintf(logContentData.data(), logContentData.size(), format, args);
	std::string logContent(logContentData.begin(), std::find_if(logContentData.begin(), logContentData.end(), [](int c) { return c == '\0'; }));

	va_end(args);

	for (const auto& listener : this->mListeners)
	{
		listener(timeStr, category, pLogger->mpTitle, logContent);
	}
}

bool LogSystem::close()
{
	assert(mpFileStream != 0);
	return fclose((FILE*)mpFileStream) == 0;
}

LogSystem::ListenerHandle LogSystem::addListener(Listener value)
{
	return this->mListeners.insert(this->mListeners.end(), value);
}

void LogSystem::removeListener(ListenerHandle &handle)
{
	this->mListeners.erase(handle);
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
