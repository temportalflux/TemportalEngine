#include "asset/AssetArchive.hpp"

#include "asset/AssetManager.hpp"
#include "utility/Archive.hpp"

using namespace asset;

std::shared_ptr<utility::OutputArchive> Archive::writer()
{
	return std::dynamic_pointer_cast<utility::OutputArchive>(this->mpActiveArchive);
}

std::shared_ptr<utility::InputArchive> Archive::reader()
{
	return std::dynamic_pointer_cast<utility::InputArchive>(this->mpActiveArchive);
}

bool Archive::startWriting(std::filesystem::path const& filePath)
{
	this->mpActiveArchive = std::make_shared<utility::OutputArchive>();
	this->writer()->start();
	this->writer()->setFormat(utility::EArchiveFormat::e7Zip);
	return this->writer()->open(filePath);
}

void Archive::writeAsset(
	std::filesystem::path const& pathInArchive,
	std::shared_ptr<asset::Asset> pAsset
)
{
	std::string binaryData;
	{
		auto os = std::stringstream(std::stringstream::out | std::stringstream::binary);
		cereal::PortableBinaryOutputArchive cerealArchive(os);
		pAsset->compile(cerealArchive, false);
		binaryData = os.str();
	}

	this->writer()->startEntry()
		.setPath(pathInArchive)
		.addPermission(utility::EPermission::eRead)
		.addPermission(utility::EPermission::eReadNamedAttributes)
		.addPermission(utility::EPermission::eReadAttributes)
		.setSize(binaryData.size())
		.finishHeader()
		.append(binaryData.data(), binaryData.size())
		.finish();
}

bool Archive::startReading(std::filesystem::path const& filePath)
{
	this->mpActiveArchive = std::make_shared<utility::InputArchive>();
	this->reader()->start();
	return this->reader()->open(filePath);
}

std::shared_ptr<asset::Asset> Archive::readNextEntry()
{
	std::shared_ptr<asset::Asset> asset;
	if (this->reader()->nextEntry())
	{
		auto binaryData = std::string(this->reader()->entrySize(), '\0');
		this->reader()->copyEntryTo(binaryData.data());

		// Determine asset type
		AssetType assetType;
		{
			auto assetBase = asset::AssetManager::makeAsset<Asset>();
			
			auto is = std::stringstream(binaryData, std::stringstream::in | std::stringstream::binary);
			cereal::PortableBinaryInputArchive cerealArchive(is);
			assetBase->decompile(cerealArchive, false);

			assetType = assetBase->getAssetType();
		}

		asset = asset::AssetManager::get()->getAssetTypeMetadata(assetType).createEmptyAsset();

		// Actually load the asset
		{
			auto is = std::stringstream(binaryData, std::stringstream::in | std::stringstream::binary);
			cereal::PortableBinaryInputArchive cerealArchive(is);
			asset->decompile(cerealArchive, false);
		}
	}
	return asset;
}

void Archive::stop()
{
	this->mpActiveArchive->close();
	this->mpActiveArchive->finish();
	this->mpActiveArchive.reset();
}
