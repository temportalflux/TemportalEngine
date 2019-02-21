// Copyright [2019] <Dustin Yost>
#include "logging/Logger.hpp"

#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <fstream>

using namespace logging;

LogSystem::LogSystem()
{
	*mpLock = TE_MutexLock();
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

void LogSystem::printPrefix(Logger *pLogger, ECategory category)
{
	const char* categoryStr;
	switch (category)
	{
		case ECategory::INFO:
			categoryStr = "INFO";
			break;
		case ECategory::WARN:
			categoryStr = "WARNING";
			break;
		case ECategory::ERROR:
			categoryStr = "ERROR";
			break;
		case ECategory::DEBUG:
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

	this->printLog("[%s][%s] %s> ", timeStr, categoryStr, pLogger->mpTitle);
}

char *convert(unsigned int num, int base)
{
	static char Representation[] = "0123456789ABCDEF";
	static char buffer[50];
	char *ptr;

	ptr = &buffer[49];
	*ptr = '\0';

	do
	{
		*--ptr = Representation[num%base];
		num /= base;
	} while (num != 0);

	return(ptr);
}

void LogSystem::open(char const * const filePath)
{
	fopen_s((FILE**)&mpFileStream, filePath, "w");
}

void LogSystem::log(Logger *pLogger, ECategory category, Message format, ...)
{
	mpLock->lock();

	this->printPrefix(pLogger, category);

	va_list args;
	va_start(args, format);
	printLog(format, args);
	printLog("\n");
	va_end(args);

	mpLock->unlock();
}

bool LogSystem::close()
{
	return fclose((FILE*)mpFileStream) == 0;
}

Logger::Logger(char const * title, LogSystem *pLogSystem)
{
	strncpy_s(mpTitle, title, strlen(title));
	mpLogSystem = pLogSystem;
}

Logger::Logger() : Logger("", 0)
{
}
