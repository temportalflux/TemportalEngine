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
	*mpLock = thread::MutexLock();
}

void LogSystem::printLog(void* pFileActive, void* pFileArchive, char const *const format, char * args)
{
	vfprintf(stdout, format, args);
	if (pFileActive != 0) vfprintf((FILE*)pFileActive, format, args);
	if (pFileArchive != 0) vfprintf((FILE*)pFileArchive, format, args);
}

void LogSystem::printLog(void* pFileActive, void* pFileArchive, char const *const format, ...)
{
	va_list args;
	va_start(args, format);
	printLog(pFileActive, pFileArchive, format, args);
	va_end(args);
}

std::string LogSystem::getCategoryShortString(ELogLevel category)
{
	switch (category)
	{
	case ELogLevel::eError: return "ERROR";
	case ELogLevel::eWarn: return "WARN ";
	case ELogLevel::eInfo: return "INFO ";
	case ELogLevel::eDebug: return "DEBUG";
	case ELogLevel::eVerbose: return "INFO2";
	case ELogLevel::eVeryVerbose: return "INFO3";
	default: return "?????";
	}
}

void LogSystem::open(std::filesystem::path archivePath)
{
	this->mActivePath = std::filesystem::current_path() / "active.log";
	this->mArchivePath = archivePath;
	if (std::filesystem::exists(this->mActivePath))
		std::filesystem::remove(this->mActivePath);
	std::filesystem::create_directories(this->mActivePath.parent_path());
	std::filesystem::create_directories(this->mArchivePath.parent_path());
}

void LogSystem::log(char const *title, ELogLevel category, Message format, ...)
{
	auto categoryStr = getCategoryShortString(category);
	auto const& timeStr = utility::currentTimeStr();
	FILE *pActive, *pArchive;

	std::array<char, 256> logContentData;
	logContentData.fill('\0');

	mpLock->lock();

	// Files are opened per log call because windows doesn't have a way to flush files to disk until `fclose` is called
	fopen_s((FILE**)&pActive, this->mActivePath.string().c_str(), "a");
	fopen_s((FILE**)&pArchive, this->mArchivePath.string().c_str(), "a");
	assert(pActive != 0 && pArchive != 0);

	// Print the log info
	printLog(
		pActive, pArchive,
		"[%s][%s] %s> ", timeStr.c_str(), categoryStr.c_str(), title
	);
	// And the log content
	va_list args;
	va_start(args, format);
	printLog(pActive, pArchive, format, args);
	vsnprintf(logContentData.data(), logContentData.size(), format, args);
	va_end(args);
	// Close out the line
	printLog(pActive, pArchive, "\n");
		
	// Flush stdout and file descriptors
	fflush(stdout);
	fclose(pActive);
	fclose(pArchive);

	std::string logContent(logContentData.begin(), std::find_if(logContentData.begin(), logContentData.end(), [](int c) { return c == '\0'; }));
	for (const auto& listener : this->mListeners)
	{
		listener(timeStr, category, title, logContent);
	}

	mpLock->unlock();
}

void LogSystem::close()
{
	this->log(
		"Log", LOG_INFO,
		"Closing log. File can be found at %s",
		this->mArchivePath.string().c_str()
	);
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
	strncpy_s(mpTitle, title, math::min(strlen(title), (uSize)LOGGER_MAX_TITLE_LENGTH));
}

Logger::Logger() : Logger("", 0)
{
}
