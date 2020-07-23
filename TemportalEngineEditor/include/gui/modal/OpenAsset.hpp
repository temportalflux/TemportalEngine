#pragma once

#include "gui/modal/Modal.hpp"

#include <array>
#include <functional>

NS_ASSET
class Asset;
NS_END

NS_GUI NS_MODAL

class OpenAsset : public Modal
{
public:
	typedef std::function<void(std::shared_ptr<asset::Asset> asset)> AssetOpenedCallback;

	OpenAsset() = default;
	OpenAsset(std::string title);

	void setDefaultPath(std::filesystem::path path);
	void setCallback(AssetOpenedCallback callback);

protected:
	void drawContents() override;

private:
	AssetOpenedCallback mOnAssetOpened;

	std::array<char, 128> mInputPath;

	void submit();

};

NS_END NS_END
