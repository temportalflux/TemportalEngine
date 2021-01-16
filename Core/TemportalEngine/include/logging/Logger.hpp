#ifndef TE_LOGGING_LOGGER_HPP
#define TE_LOGGING_LOGGER_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "thread/MutexLock.hpp"

#include <string>
#include <functional>

// ----------------------------------------------------------------------------
NS_LOGGING

/**
* Enumeration of different logging types to sort log output.
*/
enum class ELogLevel : ui8
{

	/**
	 * An expectation is not true, and this will result in game breaking crashes, bugs, or broken executions.
	 */
	eError,

	/**
	 * A expectation is not true, but the resulting behavior is not game breaking.
	 * Needs to be fixed, but wont cause crashes or other UX breaking bugs.
	 */
	eWarn,
	
	/**
	 * Logs which don't spam the log file, and which are useful for the general user.
	 */
	eInfo,

	/**
	 * Information which should only be logged in debug builds (not in release builds).
	 * Otherwise is equivalent to `eInfo`.
	 */
	eDebug,

	/**
	 * Logs which the general user won't need unless development has requested extra log information.
	 * Generally these messages can cause log spam and aren't used for general day-to-day execution.
	 */
	eVerbose,

	/**
	 * Informational messages which absolutely spam the log and are not needed unless
	 * debugging something that requires obscene amounts of log information.
	 */
	eVeryVerbose,

};

#define LOG_ERR logging::ELogLevel::eError
#define LOG_WARN logging::ELogLevel::eWarn
#define LOG_INFO logging::ELogLevel::eInfo
#define LOG_DEBUG logging::ELogLevel::eDebug
#define LOG_VERBOSE logging::ELogLevel::eVerbose
#define LOG_VERY_VERBOSE logging::ELogLevel::eVeryVerbose

// Forward Declare ------------------------------------------------------------
class Logger;
typedef char const * Message;

/**
* A thread-safe interface to actually write to files. Wrapped by Logger.
* This is meant to be used to write to a single file.
*/
class TEMPORTALENGINE_API LogSystem
{

public:
	typedef std::function<void(std::string time, ELogLevel category, std::string loggerName, std::string content)> Listener;
	typedef std::vector<Listener>::iterator ListenerHandle;

	LogSystem();

	static std::string getCategoryShortString(ELogLevel cate);

	ListenerHandle addListener(Listener value);
	void removeListener(ListenerHandle &handle);

	/**
	 * Opens a file stream for writing.
	 */
	void open(std::filesystem::path archivePath);

	/**
	* Writes output to the output streams (file and console) using variadic data.
	*/
	void log(char const *title, ELogLevel category, Message format, ...);

	/**
	* Closes the active file stream.
	*/
	void close();

private:

	std::filesystem::path mActivePath;
	std::filesystem::path mArchivePath;

	std::vector<Listener> mListeners;
	
	/** The mutexer for knowing if a different thread is currently logging. */
	thread::MutexLock mpLock[1];

	/**
	* Writes to the active output streams using known array data.
	*/
	void printLog(void* pFileActive, void* pFileArchive, char const *const format, char *args);

	/**
	* Writes to the active output streams using unknown variadic data.
	*/
	void printLog(void* pFileActive, void* pFileArchive, char const *const format, ...);

};

#define LOGGER_MAX_TITLE_LENGTH 255

/**
* Thread-safe wrapper class for writing output logs.
* See http://www.cplusplus.com/reference/cstdio/vprintf/ for formatting.
*/
class TEMPORTALENGINE_API Logger
{
	friend class LogSystem;

public:
	/**
	* @param title The title of the logger.
	* @param pLogSystem The log system to write output to.
	*/
	Logger(char const * title, LogSystem *pLogSystem);
	Logger();

	/**
	* Writes a log message using a given category to the output streams initialized by the LogSystem.
	* @param category The category to output to.
	* @param format String format as definied by fprintf (http://www.cplusplus.com/reference/cstdio/fprintf/).
	* @param args Arguments to be included via the format parameter, using fprintf format.
	*/
	template <typename... TArgs>
	void log(ELogLevel category, Message format, TArgs... args)
	{
		mpLogSystem->log(this->mpTitle, category, format, args...);
	}

private:

	/**
	* The title of the logger to forward to the Log System per call to LogSystem::log.
	*/
	char mpTitle[LOGGER_MAX_TITLE_LENGTH];

	/**
	* The log system to use to write output to.
	*/
	LogSystem *mpLogSystem;

};

NS_END

#endif