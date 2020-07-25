#include "gui/modal/OpenAsset.hpp"

#include "Editor.hpp"
#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

OpenAsset::OpenAsset(std::string title) : Modal(title)
{
	this->mConfig.root = Editor::EDITOR->getProject()->getAssetDirectory();
	this->mConfig.flags = ImGuiInputTextFlags_None;
	this->mConfig.directoryViewCfg.OnFileOpen = std::bind(&OpenAsset::onFilePicked, this, std::placeholders::_1);
}

void OpenAsset::setDefaultPath(std::filesystem::path path)
{
	this->mConfig.setPath(path);
}

void OpenAsset::setCallback(AssetOpenedCallback callback)
{
	this->mOnAssetOpened = callback;
}

void OpenAsset::onFilePicked(std::filesystem::path const &path)
{
	this->setDefaultPath(path);
	this->open();
}

void OpenAsset::drawContents()
{
	gui::renderFileSelectorField("###assetField", this->mConfig);
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
	auto asset = asset::readAssetFromDisk(this->mConfig.path(), asset::EAssetSerialization::Json);
	this->close();
	this->mOnAssetOpened(asset);
}
