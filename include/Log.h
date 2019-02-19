// Copyright [2019] <Dustin Yost>
#ifndef INCLUDE_LOG_H_
#define INCLUDE_LOG_H_

#include "Namespace.h"

NS_LOGGING

typedef char const * Owner;
typedef char const * Message;

enum class ECategory
{
	INVALID = -1,

	DEBUG,
	INFO,
	WARN,
	ERROR,

	COUNT
};

void log(Owner logger, ECategory category, Message str, ...);

#define log_info(logger, str, ...) a3_log(INFO, logger, format, __VA_ARGS__)

NS_END

#endif  // INCLUDE_LOG_H_
