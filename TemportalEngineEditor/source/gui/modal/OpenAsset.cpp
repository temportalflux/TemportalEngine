#include "gui/modal/OpenAsset.hpp"

#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

OpenAsset::OpenAsset(std::string title) : Modal(title)
{
	this->mInputPath.fill('\0');
}

void OpenAsset::setDefaultPath(std::filesystem::path path)
{
	memcpy(this->mInputPath.data(), path.string().data(), path.string().length());
}

void OpenAsset::setCallback(AssetOpenedCallback callback)
{
	this->mOnAssetOpened = callback;
}

void OpenAsset::drawContents()
{
	ImGui::InputText("Path", this->mInputPath.data(), this->mInputPath.size());
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
	auto asset = asset::readAssetFromDisk(filePath, asset::EAssetSerialization::Json);
	this->close();
	this->mOnAssetOpened(asset);
}
