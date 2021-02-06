#pragma once

#include "TemportalEnginePCH.hpp"

#include "thread/Thread.hpp"

FORWARD_DEF(NS_ASSET, class Asset);
FORWARD_DEF(NS_ASSET, class Archive);

NS_BUILD

class BuildThread
{

public:
	typedef std::vector<std::string> ErrorList;

	struct BuildState
	{
		std::filesystem::path outputPath;
		std::shared_ptr<asset::Asset> asset;
		ErrorList errors;
		bool wasSuccessful() const { return errors.empty(); }
	};

	BuildThread();
	~BuildThread();

	void start();
	bool isBuilding() const;
	std::vector<BuildState> extractState();

private:
	Thread mThread;
	bool bIsComplete;
	
	std::vector<BuildState> mBuildStates;

	bool executeBuild();
	void onBuildComplete();

};

NS_END
