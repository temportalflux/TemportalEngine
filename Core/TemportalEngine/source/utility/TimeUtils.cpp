#include "utility/TimeUtils.hpp"

#include <time.h>

std::string utility::currentTimeStr()
{
	time_t currentTime = time(nullptr);
	struct tm timeinfo;
	localtime_s(&timeinfo, &currentTime);
	auto str = std::string(20, '\0');
	strftime(str.data(), sizeof(char) * str.length(), "%Y.%m.%d-%H.%M.%S", &timeinfo);
	return str;
}
