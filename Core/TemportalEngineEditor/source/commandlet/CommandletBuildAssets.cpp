#include "commandlet/CommandletBuildAssets.hpp"

#include "Engine.hpp"
#include "Editor.hpp"
#include "build/asset/BuildAsset.hpp"
#include "asset/AssetManager.hpp"
#include "asset/Project.hpp"
#include "logging/Logger.hpp"

#include <algorithm>
#include <set>

using namespace editor;

static logging::Logger LOG = DeclareLog("Build", LOG_INFO);

void CommandletBuildAssets::initialize(utility::ArgumentMap args)
{
	LOG.log(LOG_INFO, "Building Assets...");

	auto assetManager = asset::AssetManager::get();

	{
		auto iter = args.find("project");
		if (iter == args.end() || !iter->second.has_value())
		{
			LOG.log(LOG_ERR, "No project asset provided.");
			return;
		}
		this->mPathProjectAsset = iter->second.value();
	}
	if (!std::filesystem::exists(this->mPathProjectAsset))
	{
		LOG.log(LOG_ERR, "Project file asset path does not exist.");
		return;
	}
	// TODO: get file extension from engine asset manager
	if (!std::filesystem::is_regular_file(this->mPathProjectAsset)
			|| this->mPathProjectAsset.extension() != assetManager->getAssetTypeMetadata(asset::Project::StaticType()).fileExtension)
	{
		LOG.log(LOG_ERR, "Project file asset path does not exist.");
		return;
	}
	
	{
		auto iter = args.find("outputDir");
		if (iter == args.end() || !iter->second.has_value())
		{
			LOG.log(LOG_ERR, "No output directory provided.");
			return;
		}
		this->mPathOutputDir = iter->second.value();
	}
	if (std::filesystem::exists(this->mPathOutputDir) && !std::filesystem::is_directory(this->mPathOutputDir))
	{
		LOG.log(LOG_ERR, "Output directory path is not a directory.");
		return;
	}

	// Ensure the directory now exists if it didn't before
	if (!std::filesystem::exists(this->mPathOutputDir))
	{
		std::filesystem::create_directories(this->mPathOutputDir);
	}

}

bool buildAsset(asset::AssetPtrStrong asset, std::filesystem::path outPath)
{
	auto builder = Editor::EDITOR->createAssetBuilder(asset);
	if (builder)
	{
		LOG.log(LOG_INFO, "Building: %s", asset->getPath().filename().string().c_str());

		builder->setOutputPath(outPath);
		auto errors = builder->compile(LOG);
		if (errors.empty())
		{
			builder->save();
		}
		else
		{
			for (auto error : errors)
			{
				LOG.log(LOG_ERR, error.c_str());
			}
		}
		return true;
	}
	return false;
}

void CommandletBuildAssets::run()
{
	LOG.log(LOG_INFO, "Project: %s", this->mPathProjectAsset.string().c_str());
	LOG.log(LOG_INFO, "Output: %s", this->mPathOutputDir.string().c_str());

	auto assetManager = asset::AssetManager::get();
	auto assetProj = asset::readFromDisk<asset::Project>(this->mPathProjectAsset, asset::EAssetSerialization::Json, false);
	assert(assetProj != nullptr);

	LOG.log(LOG_INFO, "Loaded project %s", assetProj->getDisplayName().c_str());

	auto assetDirSrc = assetProj->getAssetDirectory();
	auto assetDirDest = asset::Project::getAssetDirectoryFor(this->mPathOutputDir);
	std::filesystem::create_directory(assetDirDest);

	LOG.log(LOG_INFO, "Scanning for assets in %s", assetDirSrc.string().c_str());
	assetManager->scanAssetDirectory(assetDirSrc, asset::EAssetSerialization::Json);

	// the project file is just a straight save to the non-assets director
	buildAsset(assetProj, this->mPathOutputDir / this->mPathProjectAsset.filename());

	// Get a list of all pre-existing files (to delete any old ones)
	std::set<std::filesystem::path> previouslyBuiltFiles;
	{
		auto destIter = std::filesystem::recursive_directory_iterator(assetDirDest);
		auto files = std::vector<std::filesystem::directory_entry>(std::filesystem::begin(destIter), std::filesystem::end(destIter));
		std::transform(
			files.begin(), files.end(),
			std::inserter(previouslyBuiltFiles, previouslyBuiltFiles.begin()),
			[assetDirDest](auto entry) { return std::filesystem::relative(entry.path(), assetDirDest); }
		);
	}

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
		
		auto asset = asset::readAssetFromDisk(pathSrc, asset::EAssetSerialization::Json);
		if (asset != nullptr && buildAsset(asset, pathDest))
		{
			auto prevBuiltIter = previouslyBuiltFiles.find(pathRelative);
			if (prevBuiltIter != previouslyBuiltFiles.end())
			{
				previouslyBuiltFiles.erase(prevBuiltIter);
			}
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
