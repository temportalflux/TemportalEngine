#include "build/BuildThread.hpp"

#include "Engine.hpp"
#include "Editor.hpp"
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

void BuildThread::start(std::vector<std::shared_ptr<asset::Asset>> assets)
{
	this->mBuildStates.clear();

	uSize buildCount = (uSize)assets.size();
	this->mBuildStates.resize(buildCount);
	for (uIndex i = 0; i < buildCount; ++i)
	{
		this->mBuildStates[i].asset = assets[i];
	}

	this->mThread.start();
}

bool BuildThread::executeBuild()
{
	for (auto& buildState : this->mBuildStates)
	{
		buildState.errors = this->buildAsset(buildState.asset);
	}
	return false; // not iterative, we don't care about return value
}

BuildThread::ErrorList BuildThread::buildAsset(std::shared_ptr<asset::Asset> asset) const
{
	auto builder = Editor::EDITOR->createAssetBuilder(asset);
	if (builder)
	{
		builder->setOutputPath(Editor::EDITOR->getAssetBinaryPath(asset));
		auto errors = builder->compile();
		if (errors.empty()) builder->save();
		return errors;
	}
	return {};
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
