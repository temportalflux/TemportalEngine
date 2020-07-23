#pragma once

#include "gui/modal/Modal.hpp"

#include "asset/AssetType.hpp"

#include <array>
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <vector>

NS_ASSET
class Asset;
NS_END

NS_GUI NS_MODAL

class NewAsset : public Modal
{
public:
	typedef std::function<void(std::shared_ptr<asset::Asset> asset)> AssetCreatedCallback;

	NewAsset() = default;
	NewAsset(std::string title);

	void setAssetType(asset::AssetType type);
	void setCallback(AssetCreatedCallback callback);
	NewAsset& setDirectory(std::filesystem::path const &path);
	void open() override;

protected:
	void drawContents() override;

private:
	typedef std::array<char, 128> DirectoryPathString;

	AssetCreatedCallback mOnAssetCreated;

	std::optional<asset::AssetType> mForcedAssetType;

	uSize mAssetTypeCount;
	std::set<std::string> mAssetTypes;
	std::vector<std::string> mAssetTypeDisplayNames;
	void forEachAssetType(std::function<void(std::string type, std::string displayName, uSize idx)> body) const;

	std::pair<std::string, uSize> mSelectedAssetType;
	DirectoryPathString mInputDirectory;
	std::array<char, 32> mInputName;

	void submit();

};

NS_END NS_END
