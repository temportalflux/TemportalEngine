#include "asset/AssetManager.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include <fstream>
#include <cereal/archives/json.hpp>

using namespace asset;

static logging::Logger LOG = DeclareLog("AssetManager");

void AssetManager::queryAssetTypes()
{

}

struct Project
{
	std::string name;

	template<class Archive>
	void serialize(Archive &archive)
	{
		// can also use `cereal::make_nvp("name", mName)`
		archive(CEREAL_NVP(name));
	}

	std::string toString()
	{
		std::stringstream ss;
		{
			cereal::JSONOutputArchive archive(ss, cereal::JSONOutputArchive::Options::NoIndent());
			this->serialize(archive);
		}
		auto stringified = ss.str();
		// strip out all new lines
		stringified.erase(std::remove(stringified.begin(), stringified.end(), '\n'), stringified.end());
		return stringified;
	}
};

void AssetManager::createProject(std::string filePath, std::string name)
{
	auto fullFilePath = filePath + ".te-project";
	LOG.log(logging::ECategory::LOGDEBUG, "Create project with name %s at %s", name.c_str(), fullFilePath.c_str());

	Project project = { name };

	{
		std::ofstream os(fullFilePath);
		cereal::JSONOutputArchive archive(os);
		project.serialize(archive);
	}

}

void AssetManager::openProject(std::string filePath)
{
	LOG.log(logging::ECategory::LOGDEBUG, "Opening project %s", filePath.c_str());
	Project project;
	{
		std::ifstream is(filePath);
		cereal::JSONInputArchive archive(is);
		project.serialize(archive);
	}
	
	// D:\Desktop\TemportalEngine\DemoGame\DemoGame.te-project
	auto str = project.toString();
	LOG.log(logging::ECategory::LOGDEBUG, "Project: %s", str.c_str());

}
