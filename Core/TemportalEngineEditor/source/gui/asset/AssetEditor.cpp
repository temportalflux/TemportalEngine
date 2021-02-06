#include "gui/asset/AssetEditor.hpp"

#include "Editor.hpp"

#include <chrono>
#include <imgui.h>

using namespace gui;

void AssetEditor::setAsset(asset::AssetPtrStrong asset)
{
	this->mpAsset = asset;
	this->setTitle(asset->getFileName());
	this->mbIsBuildingAsset = false;
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
	// while the ui may be closed, always ensure that if there is a compilation task running, it will get cleaned up
	if (this->mbIsBuildingAsset && !Editor::EDITOR->isBuildingAssets())
	{
		// Extracts the build info and releases the build thread
		auto buildStates = Editor::EDITOR->extractBuildState();
		this->mbIsBuildingAsset = false;
		
		if (!buildStates[0].wasSuccessful() && this->isOpen())
		{
			this->onBuildFailure(buildStates[0].errors);
		}
	}
}

bool AssetEditor::shouldReleaseGui() const
{
	return !this->mbIsBuildingAsset && !this->isOpen();
}

#pragma region Asset Status

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

void AssetEditor::markAssetClean()
{
	this->mDirtyFlags = 0;
}

#pragma endregion

#pragma region Saving

void AssetEditor::saveAsset()
{
	this->mpAsset->writeToDisk(this->mpAsset->getPath(), asset::EAssetSerialization::Json);
	this->markAssetClean();
}

bool AssetEditor::hasCompiledBinary() const
{
	return std::filesystem::exists(Editor::EDITOR->getAssetBinaryPath(this->mpAsset));
}

bool AssetEditor::canCompileAsset()
{
	return !Editor::EDITOR->isBuildingAssets();
}

void AssetEditor::compileAsset()
{
	this->saveAsset();

	this->mbIsBuildingAsset = true;
	Editor::EDITOR->buildAllAssets();
}

void AssetEditor::onBuildFailure(std::vector<std::string> const &errors)
{
}

#pragma endregion

void AssetEditor::renderView()
{
	this->renderMenuBar();

	if (this->getDetailsPanelWidth() > 0 && this->mbDetailsPanelOpen)
	{
		ImGui::BeginChild("Details", ImVec2(this->getDetailsPanelWidth(), 0), false);
		this->renderDetailsPanel();
		ImGui::EndChild();
		ImGui::SameLine();
	}
	{
		ImGui::BeginChild("Content", ImVec2(0, 0), false);
		this->renderContent();
		ImGui::EndChild();
	}
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
	if (ImGui::MenuItem("Compile", "", false, this->canCompileAsset())) this->compileAsset();
}

void AssetEditor::renderContent()
{
	if (this->getDetailsPanelWidth() > 0)
	{
		if (ImGui::ArrowButton("ToggleDetailsPanel",
			this->mbDetailsPanelOpen ? ImGuiDir_Left : ImGuiDir_Right
		)) this->mbDetailsPanelOpen = !this->mbDetailsPanelOpen;
		ImGui::SameLine();
	}
	this->renderBinaryInformation();
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
