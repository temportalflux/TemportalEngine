#pragma once

#include "TemportalEnginePCH.hpp"

void deleteOldestFiles(std::filesystem::path directory, uSize amountToKeep)
{
	auto allRegularFiles = std::vector<std::filesystem::path>();
	for (auto entry : std::filesystem::directory_iterator(directory))
	{
		if (!entry.is_regular_file()) continue;
		allRegularFiles.insert(
			std::lower_bound(
				allRegularFiles.begin(), allRegularFiles.end(),
				entry.last_write_time(),
				[](std::filesystem::path const& filePath, std::filesystem::file_time_type const& lastWriteTime)
		{
			return std::filesystem::last_write_time(filePath) < lastWriteTime;
		}
			),
			entry.path()
			);
	}
	uSize amountToRemove = allRegularFiles.size() - amountToKeep;
	auto oldestRegularFilePath = allRegularFiles.begin();
	while (oldestRegularFilePath != allRegularFiles.end() && amountToRemove > 0)
	{
		std::filesystem::remove(*oldestRegularFilePath);
		++oldestRegularFilePath;
		--amountToRemove;
	}
}
