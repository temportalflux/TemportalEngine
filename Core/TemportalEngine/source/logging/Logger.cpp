#include "logging/Logger.hpp"

// Libraries ------------------------------------------------------------------
#include <cassert>
#include <fstream>
#include <stdarg.h>
#include <array>
#include <cereal/types/map.hpp>

// Engine ---------------------------------------------------------------------
#include "math/compare.h"
#include "utility/TimeUtils.hpp"

// Namespaces -----------------------------------------------------------------
using namespace logging;

bool LevelConfig::has(std::string id) const
{
	return this->levelByLogId.find(id) != this->levelByLogId.end();
}

logging::ELogLevel LevelConfig::get(std::string id) const
{
	if (this->has(id)) return this->levelByLogId.find(id)->second;
	else return ELogLevel::eVeryVerbose;
}

namespace cereal
{

	std::string save_minimal(cereal::JSONOutputArchive const& archive, ELogLevel const &value)
	{
		switch (value)
		{
		case ELogLevel::eError: return "Error";
		case ELogLevel::eWarn: return "Warning";
		case ELogLevel::eDebug: return "Debug";
		case ELogLevel::eInfo: return "Info";
		case ELogLevel::eVerbose: return "Verbose";
		case ELogLevel::eVeryVerbose: return "VeryVerbose";
		default: return "invalid";
		}
	}

	void load_minimal(cereal::JSONInputArchive const& archive, ELogLevel &value, std::string const& out)
	{
		if (out == "Error") value = LOG_ERR;
		else if (out == "Warning") value = LOG_WARN;
		else if (out == "Debug") value = LOG_DEBUG;
		else if (out == "Info") value = LOG_INFO;
		else if (out == "Verbose") value = LOG_VERBOSE;
		else if (out == "VeryVerbose") value = LOG_VERY_VERBOSE;
		else value = LOG_INFO;
	}

}

cereal::JSONOutputArchive::Options JsonFormat = cereal::JSONOutputArchive::Options(
	324,
	cereal::JSONOutputArchive::Options::IndentChar::tab, 1
);

void initializeLevels(LevelConfig &cfg, std::filesystem::path cfgPath)
{
	std::filesystem::create_directories(cfgPath.parent_path());
	if (!std::filesystem::exists(cfgPath))
	{
		std::ofstream os(cfgPath.string());
		cereal::JSONOutputArchive archive(os, JsonFormat);
		cfg.save(archive);
	}
	std::ifstream is(cfgPath.string());
	cereal::JSONInputArchive archive(is);
	cfg.load(archive);
}

void LevelConfig::save(cereal::JSONOutputArchive &archive) const
{
	archive.setNextName("levels");
	archive.startNode();
	for (auto const& [id, level] : this->levelByLogId)
	{
		archive(cereal::make_nvp(id.c_str(), level));
	}
	archive.finishNode();
}

void LevelConfig::load(cereal::JSONInputArchive &archive)
{
	archive.startNode();
	char const* nodeName = nullptr;
	while (nodeName = archive.getNodeName())
	{
		std::string levelStr;
		archive.loadValue(levelStr);
		ELogLevel level;
		cereal::load_minimal(archive, level, levelStr);
		this->levelByLogId.insert(std::make_pair(nodeName, level));
	}
	archive.finishNode();
}

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

std::string LogSystem::getLevelString(ELogLevel level)
{
	switch (level)
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

void LogSystem::open(
	std::filesystem::path archivePath,
	std::filesystem::path logLevelsConfigPath
)
{
	initializeLevels(this->mLevels, logLevelsConfigPath);

	this->mActivePath = std::filesystem::current_path() / "active.log";
	this->mArchivePath = archivePath;
	if (std::filesystem::exists(this->mActivePath))
		std::filesystem::remove(this->mActivePath);
	std::filesystem::create_directories(this->mActivePath.parent_path());
	std::filesystem::create_directories(this->mArchivePath.parent_path());
}

void LogSystem::log(std::string const& title, ELogLevel threshold, ELogLevel level, Message format, ...)
{
	if (this->mLevels.has(title))
	{
		threshold = this->mLevels.get(title);
	}
	if (ui8(level) > ui8(threshold)) return;

	auto const categoryStr = LogSystem::getLevelString(level);
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
		"[%s][%s] %s> ", timeStr.c_str(), categoryStr.c_str(), title.c_str()
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
		listener(timeStr, level, title, logContent);
	}

	mpLock->unlock();
}

void LogSystem::close()
{
	this->log(
		"Log", LOG_INFO, LOG_INFO,
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

Logger::Logger(std::string title, ELogLevel defaultThreshold, LogSystem *pLogSystem)
	: mpLogSystem(pLogSystem)
	, mTitle(title)
	, mDefaultThreshold(defaultThreshold)
{
}

Logger::Logger() : Logger("", ELogLevel::eInfo, 0)
{
}
