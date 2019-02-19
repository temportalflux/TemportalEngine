// Copyright [2019] <Dustin Yost>
#include "Log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

NS_LOGGING

void printPrefix(ECategory category, Owner logger)
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

	printf("[%s][%s] %s> ", timeStr, categoryStr, logger);
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

void log(Owner logger, ECategory category, Message format, ...)
{
	printPrefix(category, logger);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

NS_END
