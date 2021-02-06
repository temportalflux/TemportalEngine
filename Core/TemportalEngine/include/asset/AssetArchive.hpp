#pragma once

#include "asset/Asset.hpp"

FORWARD_DEF(NS_UTILITY, class Archive);
FORWARD_DEF(NS_UTILITY, class OutputArchive);
FORWARD_DEF(NS_UTILITY, class InputArchive);

NS_ASSET

/**
 * An Asset Archive is a package (zip/pak) file containing binary serializations of assets.
 */
class Archive
{

public:
	
	bool startWriting(std::filesystem::path const& filePath);
	void writeAsset(
		std::filesystem::path const& pathInArchive, 
		std::shared_ptr<asset::Asset> pAsset
	);
	
	bool startReading(std::filesystem::path const& filePath);
	std::shared_ptr<asset::Asset> readNextEntry();

	void stop();

private:
	std::shared_ptr<utility::Archive> mpActiveArchive;

	std::shared_ptr<utility::OutputArchive> writer();
	std::shared_ptr<utility::InputArchive> reader();

};

NS_END
