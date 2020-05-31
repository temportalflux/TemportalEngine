#include "gui/modal/OpenAsset.hpp"

#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

OpenAsset::OpenAsset() : Modal("Open Asset")
{
	this->mInputPath.fill('\0');
}

void OpenAsset::drawContents()
{
	ImGui::InputText("Directory", this->mInputPath.data(), this->mInputPath.size());
	if (ImGui::Button("Open"))
	{
		this->submit();
	}
}

void OpenAsset::submit()
{
	auto filePath = utility::createStringFromFixedArray(this->mInputPath);
	auto asset = asset::AssetManager::get()->readAssetFromDisk(filePath);
	asset.reset(); // immediately release the asset
	this->close();
}

void OpenAsset::reset()
{
	Modal::reset();
	this->mInputPath.fill('\0');
}
