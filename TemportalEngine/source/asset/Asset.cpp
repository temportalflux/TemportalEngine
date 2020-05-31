#include "asset/Asset.hpp"

#include <algorithm>
#include <cereal/archives/json.hpp>

using namespace asset;

std::shared_ptr<Asset> Asset::readAsset(std::ifstream *stream)
{
	auto ptr = std::make_shared<Asset>();
	cereal::JSONInputArchive archive(*stream);
	ptr->load(archive);
	return ptr;
}

/*
std::string Asset::toString()
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive archive(ss, cereal::JSONOutputArchive::Options::NoIndent());
		//std::shared_ptr<Asset> asset = std::make_shared<Asset>(this);
		//archive(cereal::make_nvp(asset->getAssetType(), asset));
	}
	auto stringified = ss.str();
	// strip out all new lines
	stringified.erase(std::remove(stringified.begin(), stringified.end(), '\n'), stringified.end());
	return stringified;
}
//*/
