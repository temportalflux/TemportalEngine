#pragma once

#include "gui/modal/Modal.hpp"

#include "types/integer.h"

#include <array>
#include <functional>
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

	NewAsset();

	void open() override;
	void setCallback(AssetCreatedCallback callback);

protected:
	void drawContents() override;
	void reset() override;

private:

	AssetCreatedCallback mOnAssetCreated;

	uSize mAssetTypeCount;
	std::set<std::string> mAssetTypes;
	std::vector<std::string> mAssetTypeDisplayNames;
	void forEachAssetType(std::function<void(std::string type, std::string displayName, uSize idx)> body) const;

	std::pair<std::string, uSize> mSelectedAssetType;
	std::array<char, 128> mInputDirectory;
	std::array<char, 32> mInputName;

	void submit();

};

NS_END NS_END
