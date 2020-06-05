#include "gui/asset/AssetEditor.hpp"

#include "Editor.hpp"

#include <chrono>
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
	this->renderBinaryInformation();
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

std::string makeTimeString(std::filesystem::file_time_type time)
{
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
	);
	auto stdTime = std::chrono::system_clock::to_time_t(sctp);
	struct tm timeinfo;
	localtime_s(&timeinfo, &stdTime);
	char timeStr[20]; // ####-##-## ##:##:##\0
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
	return std::string(timeStr);
}

void AssetEditor::renderBinaryInformation()
{
	auto binaryPath = Editor::EDITOR->getAssetBinaryPath(this->mpAsset);
	if (!std::filesystem::exists(binaryPath) || !std::filesystem::is_regular_file(binaryPath))
	{
		ImGui::Text("No binary asset compiled");
		return;
	}
	ImGui::Text("Last saved:");
	ImGui::SameLine();
	ImGui::Text(makeTimeString(std::filesystem::last_write_time(binaryPath)).c_str());
}
