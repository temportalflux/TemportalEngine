#include "asset/Project.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

using namespace asset;

std::shared_ptr<Asset> Project::createAsset(std::filesystem::path filePath)
{
	// TODO: use std::allocate_shared instead of make_shared and utilize the memory system http://www.cplusplus.com/reference/memory/allocate_shared/
	auto ptr = std::make_shared<Project>();
	ptr->mName = filePath.stem().string();
	ptr->mProjectDirectory = filePath.parent_path();

	std::ofstream os(filePath);
	ptr->writeToDisk(&os);

	return ptr;
}

std::shared_ptr<Asset> Project::readFromDisk(std::ifstream *stream, std::filesystem::path filePath)
{
	auto ptr = std::make_shared<Project>();
	cereal::JSONInputArchive archive(*stream);
	ptr->load(archive);
	ptr->mProjectDirectory = filePath.parent_path();
	return ptr;
}

void Project::writeToDisk(std::ofstream *stream)
{
	cereal::JSONOutputArchive archive(*stream);
	this->save(archive);
}
