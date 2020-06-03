#include "gui/asset/AssetEditor.hpp"

#include "Editor.hpp"

#include <imgui.h>

using namespace gui;

void AssetEditor::setAsset(asset::AssetPtrStrong asset)
{
	this->mpAsset = asset;
	this->setTitle(asset->getFileName());
}

std::string AssetEditor::getId() const
{
	return this->mpAsset->getPath().string();
}

std::string AssetEditor::getTitle() const
{
	return this->mpAsset->getFileName() + (this->isAssetDirty() ? " *" : "");
}

i32 AssetEditor::getFlags() const
{
	return ImGuiWindowFlags_MenuBar;
}

void AssetEditor::makeGui()
{
	IGui::makeGui();
	if (!this->isOpen())
	{
		Editor::EDITOR->closeGui(this->mpAsset->getPath().string());
		this->releaseAsset();
	}
}

bool AssetEditor::isAssetDirty() const
{
	return this->mDirtyFlags > 0;
}

void AssetEditor::markAssetDirty(ui32 bit, bool isDirty)
{
	this->mDirtyFlags = isDirty ? this->mDirtyFlags | bit : this->mDirtyFlags & (~bit);
}

void AssetEditor::saveAsset()
{
	this->mpAsset->writeToDisk(this->mpAsset->getPath(), asset::EAssetSerialization::Json);
	this->mDirtyFlags = 0;
}

void AssetEditor::releaseAsset()
{
	this->mpAsset.reset();
}

void AssetEditor::renderView()
{
	this->renderMenuBar();
}

void AssetEditor::renderMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::MenuItem("Save", "", false, this->isAssetDirty())) this->saveAsset();
		ImGui::EndMenuBar();
	}
}
