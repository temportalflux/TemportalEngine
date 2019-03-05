#ifndef TE_LOGGING_LOGGER_HPP
#define TE_LOGGING_LOGGER_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "thread/MutexLock.hpp"

// ----------------------------------------------------------------------------
NS_LOGGING

/**
* Enumeration of different logging types to sort log output.
*/
enum class TEMPORTALENGINE_API ECategory
{
	INVALID = -1,

	/** Output which will not be ignored in a release executable. */
	LOGDEBUG,
	/** Important, but not critical or bug related. */
	LOGINFO,
	/** Likely a bug or unintended behavior. */
	LOGWARN,
	/** A game breaking bug and potentially game breaking (pun intended). */
	LOGERROR,

	COUNT
};

// Forward Declare ------------------------------------------------------------
class Logger;
typedef char const * Message;

/**
* A thread-safe interface to actually write to files. Wrapped by Logger.
* This is meant to be used to write to a single file.
*/
class TEMPORTALENGINE_API LogSystem
{

private:
	
	/** The mutexer for knowing if a different thread is currently logging. */
	thread::MutexLock mpLock[1];

	/** The standard lib file stream */
	void* mpFileStream;

	/**
	* Writes to the active output streams using known array data.
	*/
	void printLog(char const *const format, char *args);

	/**
	* Writes to the active output streams using unknown variadic data.
	*/
	void printLog(char const *const format, ...);

public:

	LogSystem();
	
	/**
	* Opens a file stream for writing.
	*/
	void open(char const *const filePath);
	
	/**
	* Writes output to the output streams (file and console) using variadic data.
	*/
	void log(Logger *pLogger, ECategory category, Message format, ...);
	
	/**
	* Closes the active file stream.
	*/
	bool close();

};

#define LOGGER_MAX_TITLE_LENGTH 255

/**
* Thread-safe wrapper class for writing output logs.
*/
class TEMPORTALENGINE_API Logger
{
	friend class LogSystem;

private:

	/**
	* The title of the logger to forward to the Log System per call to LogSystem::log.
	*/
	char mpTitle[LOGGER_MAX_TITLE_LENGTH];
	
	/**
	* The log system to use to write output to.
	*/
	LogSystem *mpLogSystem;

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
	void log(ECategory category, Message format, TArgs... args)
	{
		mpLogSystem->log(this, category, format, args...);
	}

};

/**
* A raw wrapper for the Logger class which creates a logger on the fly and outputs it.
* Not recommended for long term use, but useful in a one-off situtation.
*/
template <typename... TArgs>
void log(char const *title, LogSystem *pLogSystem, ECategory category, Message format, TArgs... args)
{
	Logger(title, pLogSystem).log(category, format, args...);
}

NS_END

#endif