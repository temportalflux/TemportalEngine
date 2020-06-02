#include "commandlet/CommandletBuildAssets.hpp"

#include "Engine.hpp"
#include "asset/AssetManager.hpp"
#include "asset/Project.hpp"
#include "logging/Logger.hpp"

#include <filesystem>

using namespace editor;

void CommandletBuildAssets::run(utility::ArgumentMap args)
{
	static logging::Logger LOG = DeclareLog("CommandletBuildAssets");
	LOG.log(logging::ECategory::LOGINFO, "Building Assets...");

	auto assetManager = asset::AssetManager::get();

	std::filesystem::path projectFilePath;
	{
		auto iter = args.find("project");
		if (iter == args.end() || !iter->second.has_value())
		{
			LOG.log(logging::ECategory::LOGERROR, "No project asset provided.");
			return;
		}
		projectFilePath = iter->second.value();
	}
	if (!std::filesystem::exists(projectFilePath))
	{
		LOG.log(logging::ECategory::LOGERROR, "Project file asset path does not exist.");
		return;
	}
	// TODO: get file extension from engine asset manager
	if (!std::filesystem::is_regular_file(projectFilePath)
			|| projectFilePath.extension() != assetManager->getAssetTypeMetadata(AssetType_Project).fileExtension)
	{
		LOG.log(logging::ECategory::LOGERROR, "Project file asset path does not exist.");
		return;
	}
	
	std::filesystem::path outputDirPath;
	{
		auto iter = args.find("outputDir");
		if (iter == args.end() || !iter->second.has_value())
		{
			LOG.log(logging::ECategory::LOGERROR, "No project asset provided.");
			return;
		}
		outputDirPath = iter->second.value();
	}
	if (std::filesystem::exists(outputDirPath) && !std::filesystem::is_directory(outputDirPath))
	{
		LOG.log(logging::ECategory::LOGERROR, "Output directory path is not a directory.");
		return;
	}

	// Ensure the directory now exists if it didn't before
	if (!std::filesystem::exists(outputDirPath))
	{
		std::filesystem::create_directories(outputDirPath);
	}

}
