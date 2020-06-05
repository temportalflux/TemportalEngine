#include "commandlet/CommandletBuildAssets.hpp"

#include "Engine.hpp"
#include "asset/AssetManager.hpp"
#include "asset/Project.hpp"
#include "logging/Logger.hpp"

#include <algorithm>
#include <set>

using namespace editor;

static logging::Logger LOG = DeclareLog("Build");

void CommandletBuildAssets::initialize(utility::ArgumentMap args)
{
	LOG.log(logging::ECategory::LOGINFO, "Building Assets...");

	auto assetManager = asset::AssetManager::get();

	{
		auto iter = args.find("project");
		if (iter == args.end() || !iter->second.has_value())
		{
			LOG.log(logging::ECategory::LOGERROR, "No project asset provided.");
			return;
		}
		this->mPathProjectAsset = iter->second.value();
	}
	if (!std::filesystem::exists(this->mPathProjectAsset))
	{
		LOG.log(logging::ECategory::LOGERROR, "Project file asset path does not exist.");
		return;
	}
	// TODO: get file extension from engine asset manager
	if (!std::filesystem::is_regular_file(this->mPathProjectAsset)
			|| this->mPathProjectAsset.extension() != assetManager->getAssetTypeMetadata(AssetType_Project).fileExtension)
	{
		LOG.log(logging::ECategory::LOGERROR, "Project file asset path does not exist.");
		return;
	}
	
	{
		auto iter = args.find("outputDir");
		if (iter == args.end() || !iter->second.has_value())
		{
			LOG.log(logging::ECategory::LOGERROR, "No output directory provided.");
			return;
		}
		this->mPathOutputDir = iter->second.value();
	}
	if (std::filesystem::exists(this->mPathOutputDir) && !std::filesystem::is_directory(this->mPathOutputDir))
	{
		LOG.log(logging::ECategory::LOGERROR, "Output directory path is not a directory.");
		return;
	}

	// Ensure the directory now exists if it didn't before
	if (!std::filesystem::exists(this->mPathOutputDir))
	{
		std::filesystem::create_directories(this->mPathOutputDir);
	}

}

void CommandletBuildAssets::run()
{
	LOG.log(logging::ECategory::LOGINFO, "Project: %s", this->mPathProjectAsset.string().c_str());
	LOG.log(logging::ECategory::LOGINFO, "Output: %s", this->mPathOutputDir.string().c_str());

	auto assetManager = asset::AssetManager::get();
	auto assetProj = assetManager->readFromDisk<asset::Project>(this->mPathProjectAsset, asset::EAssetSerialization::Json, false);
	assert(assetProj != nullptr);

	LOG.log(logging::ECategory::LOGINFO, "Loaded project %s", assetProj->getDisplayName().c_str());

	// the project file is just a straight save to the non-assets director
	assetProj->writeToDisk(this->mPathOutputDir / this->mPathProjectAsset.filename(), asset::EAssetSerialization::Binary);

	auto assetDirSrc = assetProj->getAssetDirectory();
	auto assetDirDest = asset::Project::getAssetDirectoryFor(this->mPathOutputDir);
	std::filesystem::create_directory(assetDirDest);

	LOG.log(logging::ECategory::LOGINFO, "Scanning for assets in %s", assetDirSrc.string().c_str());
	assetManager->scanAssetDirectory(assetDirSrc, asset::EAssetSerialization::Json);

	// Get a list of all pre-existing files (to delete any old ones)
	std::set<std::filesystem::path> previouslyBuiltFiles;
	{
		auto destIter = std::filesystem::recursive_directory_iterator(assetDirDest);
		auto files = std::vector<std::filesystem::directory_entry>(std::filesystem::begin(destIter), std::filesystem::end(destIter));
		/*
		files.erase(std::remove_if(
			files.begin(), files.end(),
			[](const std::filesystem::directory_entry entry) { return entry.is_directory(); }
		), files.end());
		//*/
		std::transform(
			files.begin(), files.end(),
			std::inserter(previouslyBuiltFiles, previouslyBuiltFiles.begin()),
			[assetDirDest](auto entry) { return std::filesystem::relative(entry.path(), assetDirDest); }
		);
	}

	LOG.log(logging::ECategory::LOGINFO, "Compiling assets...");
	for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDirSrc))
	{
		auto pathSrc = entry.path();
		auto pathRelative = std::filesystem::relative(pathSrc, assetDirSrc);
		auto pathDest = assetDirDest / pathRelative;
		if (std::filesystem::is_directory(pathSrc))
		{
			std::filesystem::create_directory(pathDest);
			continue;
		}

		auto prevBuiltIter = previouslyBuiltFiles.find(pathRelative);
		if (prevBuiltIter != previouslyBuiltFiles.end())
		{
			previouslyBuiltFiles.erase(prevBuiltIter);
		}
		
		// TODO: confirm that each asset's serialization functions are correctly called
		auto asset = assetManager->readAssetFromDisk(pathSrc, asset::EAssetSerialization::Json);
		if (asset != nullptr)
		{
			// TODO: Needs to support shaders like the EditorShader does (asynchronous asset compilation)
			LOG.log(logging::ECategory::LOGINFO, "Compiling %s", pathRelative.string().c_str());
			asset->writeToDisk(pathDest, asset::EAssetSerialization::Binary);
		}
		else
		{
			LOG.log(logging::ECategory::LOGINFO, "Copying %s", pathRelative.string().c_str());
			std::filesystem::copy(pathSrc, pathDest, std::filesystem::copy_options::overwrite_existing);
		}
	}

	// Delete all the files which were not built but had been there previously
	for (auto iter = previouslyBuiltFiles.rbegin(); iter != previouslyBuiltFiles.rend(); ++iter)
	{
		auto unbuiltFile = *iter;
		auto destPath = assetDirDest / unbuiltFile;
		if (!std::filesystem::is_directory(destPath) && std::filesystem::is_empty(destPath))
		{
			std::filesystem::remove(destPath);
			previouslyBuiltFiles.erase(unbuiltFile);
		}
	}
	// then revisit for directories (for there may have been some files in the directories)
	for (const auto& dir : previouslyBuiltFiles)
	{
		auto destPath = assetDirDest / dir;
		if (std::filesystem::is_empty(destPath))
		{
			std::filesystem::remove(destPath);
		}
	}

}
