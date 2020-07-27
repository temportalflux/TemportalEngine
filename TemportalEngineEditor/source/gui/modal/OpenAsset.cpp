#include "gui/modal/OpenAsset.hpp"

#include "Editor.hpp"
#include "Engine.hpp"
#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

OpenAsset::OpenAsset(std::string title) : Modal(title)
{
	this->mConfig.root = std::filesystem::current_path().root_directory();
	this->mConfig.flags = ImGuiInputTextFlags_None;
	this->mConfig.directoryViewCfg.OnFileOpen = std::bind(&OpenAsset::onFilePicked, this, std::placeholders::_1);
	this->mConfig.directoryViewCfg.CanShowFile = std::bind(&OpenAsset::canShowFileInPicker, this, std::placeholders::_1);
}

void OpenAsset::setRoot(std::filesystem::path const path)
{
	this->mConfig.root = path;
}

void OpenAsset::setDefaultPath(std::filesystem::path path)
{
	this->mConfig.setPath(path);
}

void OpenAsset::setCallback(AssetOpenedCallback callback)
{
	this->mOnAssetOpened = callback;
}

void OpenAsset::addAssetType(asset::AssetType const &type)
{
	this->mValidAssetTypes.insert(type);
}

bool OpenAsset::canShowFileInPicker(std::filesystem::path const &path)
{
	auto extension = path.extension().string();
	auto assetManager = engine::Engine::Get()->getAssetManager();
	if (!assetManager->isValidAssetExtension(extension)) return false;
	auto assetPath = assetManager->getAssetMetadata(path);
	if (!assetPath) return true; // allow the showing of assets that aren't scanned yet
	return this->mValidAssetTypes.find(assetPath->type()) != this->mValidAssetTypes.end();
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
	if (this->mValidAssetTypes.empty() || this->mValidAssetTypes.find(asset->getAssetType()) != this->mValidAssetTypes.end())
	{
		this->close();
		this->mOnAssetOpened(asset);
	}
}
