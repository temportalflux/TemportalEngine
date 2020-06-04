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

ui32 AssetEditor::getDirtyFlags() const
{
	return this->mDirtyFlags;
}

bool AssetEditor::isAssetDirty() const
{
	return this->getDirtyFlags() != 0;
}

void AssetEditor::markAssetDirty(ui32 bit, bool isDirty)
{
	this->mDirtyFlags = isDirty ? this->mDirtyFlags | bit : this->mDirtyFlags & (~bit);
}

bool AssetEditor::isBitDirty(ui32 bit) const
{
	return (this->getDirtyFlags() & bit) == bit;
}

void AssetEditor::saveAsset()
{
	this->mpAsset->writeToDisk(this->mpAsset->getPath(), asset::EAssetSerialization::Json);
	this->markAssetClean();
}

void AssetEditor::markAssetClean()
{
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
		this->renderMenuBarItems();
		ImGui::EndMenuBar();
	}
}

void AssetEditor::renderMenuBarItems()
{
		if (ImGui::MenuItem("Save", "", false, this->isAssetDirty())) this->saveAsset();
}
