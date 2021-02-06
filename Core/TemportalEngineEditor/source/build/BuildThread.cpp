#include "build/BuildThread.hpp"

#include "Engine.hpp"
#include "Editor.hpp"
#include "asset/AssetArchive.hpp"
#include "build/asset/BuildAsset.hpp"

using namespace build;

BuildThread::BuildThread()
{
	this->mThread = Thread("BuildThread", &engine::Engine::LOG_SYSTEM);
	this->mThread.setFunctor(std::bind(&BuildThread::executeBuild, this), false);
	this->mThread.setOnComplete(std::bind(&BuildThread::onBuildComplete, this));
}

BuildThread::~BuildThread()
{
	if (this->mThread.isActive())
		this->mThread.join();
}

void BuildThread::start()
{
	this->mBuildStates.clear();
	this->mThread.start();
}

bool BuildThread::executeBuild()
{
	auto pProject = Editor::EDITOR->getProject();
	auto projectName = pProject->getName();
	auto projectPackageDir = Editor::EDITOR->getOutputDirectory() / "packages" / projectName;
	std::filesystem::create_directories(projectPackageDir);

	// TEMPORARY until all binary asset reading comes from pak files
	{
		auto assetDirDest = asset::Project::getAssetDirectoryFor(Editor::EDITOR->getOutputDirectory());
		if (std::filesystem::exists(assetDirDest))
		{
			std::filesystem::remove_all(assetDirDest);
		}
	}

	auto assetPak = asset::Archive();
	assetPak.startWriting(projectPackageDir / (projectName + ".pak"));

	this->mBuildStates = std::vector<BuildState>();
	this->mBuildStates.push_back({ pProject->getPath().filename(), pProject });
	for (const auto& entry : std::filesystem::recursive_directory_iterator(pProject->getAssetDirectory()))
	{
		if (entry.is_directory()) continue;

		BuildState buildState;
		buildState.asset = asset::readAssetFromDisk(entry.path(), asset::EAssetSerialization::Json);
		if (buildState.asset == nullptr) continue;

		buildState.outputPath = std::filesystem::relative(
			entry.path(),
			pProject->getAssetDirectory()
		);
		
		this->mBuildStates.push_back(buildState);
	}
	for (auto& buildState : this->mBuildStates)
	{
		auto builder = Editor::EDITOR->createAssetBuilder(buildState.asset);
		if (!builder)
		{
			buildState.errors.push_back("No builder available");
			continue;
		}

		// TEMPORARY until all binary asset reading comes from pak files
		builder->setOutputPath(Editor::EDITOR->getAssetBinaryPath(buildState.asset));

		buildState.errors = builder->compile(this->mThread.logger());
		if (buildState.wasSuccessful())
		{
			assetPak.writeAsset(buildState.outputPath, buildState.asset);
			// TEMPORARY until all binary asset reading comes from pak files
			builder->save();
		}
	}
	
	assetPak.stop();

	return false; // not iterative, we don't care about return value
}

void BuildThread::onBuildComplete()
{
	this->bIsComplete = true;
}

// Main thread for checking on the task thread
bool BuildThread::isBuilding() const
{
	return this->mThread.isValid() && !this->mBuildStates.empty() && this->mThread.isActive();
}

std::vector<BuildThread::BuildState> BuildThread::extractState()
{
	this->mThread.join();
	return std::move(this->mBuildStates);
}
