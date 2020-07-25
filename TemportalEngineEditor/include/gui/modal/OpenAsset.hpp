#pragma once

#include "gui/modal/Modal.hpp"

#include "gui/widget/filesystem.hpp"

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
	gui::FileSelectorField mConfig;
	AssetOpenedCallback mOnAssetOpened;

	void submit();
	void onFilePicked(std::filesystem::path const &path);

};

NS_END NS_END
