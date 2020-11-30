#pragma once

#include "TemportalEnginePCH.hpp"

#include "thread/Thread.hpp"

FORWARD_DEF(NS_ASSET, class Asset);

NS_BUILD

class BuildThread
{

public:
	typedef std::vector<std::string> ErrorList;

	struct BuildState
	{
		std::shared_ptr<asset::Asset> asset;
		ErrorList errors;
		bool wasSuccessful() const { return errors.empty(); }
	};

	BuildThread();
	~BuildThread();

	/**
	 * Builds all provided assets on a thread.
	 */
	void start(std::vector<std::shared_ptr<asset::Asset>> assets);

	bool isBuilding() const;
	std::vector<BuildState> extractState();

private:
	Thread mThread;
	bool bIsComplete;
	
	std::vector<BuildState> mBuildStates;

	bool executeBuild();
	ErrorList buildAsset(std::shared_ptr<asset::Asset> asset);
	void onBuildComplete();

};

NS_END
