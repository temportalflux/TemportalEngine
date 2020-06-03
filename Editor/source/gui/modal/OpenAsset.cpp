#include "gui/modal/OpenAsset.hpp"

#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

OpenAsset::OpenAsset(char const *title) : Modal(title)
{
	this->mInputPath.fill('\0');
	// TODO: Remove this when we have a filepicker
	auto defaultProject = std::filesystem::absolute("../../DemoGame/DemoGame.te-project");
	memcpy(this->mInputPath.data(), defaultProject.string().data(), defaultProject.string().length());
}

void OpenAsset::setCallback(AssetOpenedCallback callback)
{
	this->mOnAssetOpened = callback;
}

void OpenAsset::drawContents()
{
	ImGui::InputText("Directory", this->mInputPath.data(), this->mInputPath.size());
	if (ImGui::Button("Open"))
	{
		this->submit();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		this->close();
	}
}

void OpenAsset::submit()
{
	auto filePath = utility::createStringFromFixedArray(this->mInputPath);
	auto asset = asset::AssetManager::get()->readAssetFromDisk(filePath, asset::EAssetSerialization::Json);
	this->close();
	this->mOnAssetOpened(asset);
}

void OpenAsset::reset()
{
	Modal::reset();
	this->mInputPath.fill('\0');
}
