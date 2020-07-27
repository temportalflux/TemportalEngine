#include "gui/MainDockspace.hpp"

#include "Editor.hpp"
#include "graphics/ImGuiRenderer.hpp"
#include "memory/MemoryChunk.hpp"

#include <imgui.h>

using namespace gui;

static const char* MODAL_ID_NEW_PROJECT = "new_project";

MainDockspace::MainDockspace(std::string id, std::string title)
	: IGui(title), mId(id), mbIsBuildingAssets(false)
{
	this->mOnProjectOpenedOrCreated = [](asset::AssetPtrStrong asset)
	{
		Editor::EDITOR->setProject(asset);
	};
	this->mOnAssetCreated = [](asset::AssetPtrStrong asset)
	{
		Editor::EDITOR->openAssetEditor(asset);
	};
}

void MainDockspace::makeGui()
{
	IGui::makeGui();

	if (this->mbIsBuildingAssets && !Editor::EDITOR->isBuildingAssets())
	{
		// Extracts the build info and releases the build thread
		auto buildStates = Editor::EDITOR->extractBuildState();
		this->mbIsBuildingAssets = false;

		std::string errorMessage = "Build Errors:\n";
		bool bHadBuildErrors = false;
		for (auto& state : buildStates)
		{
			if (!state.wasSuccessful())
			{
				bHadBuildErrors = true;
				for (auto err : state.errors)
				{
					errorMessage += std::string("[") + state.asset->getFileName() + "] " + err + "\n";
				}
			}
		}
		if (bHadBuildErrors)
		{
			this->getLog()->log(LOG_ERR, errorMessage.c_str());
		}
	}
}

i32 MainDockspace::getFlags() const
{
	return
		ImGuiWindowFlags_MenuBar
		| (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)
		| (ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
}

bool MainDockspace::beginView()
{
	ImGuiIO& io = ImGui::GetIO();
	assert(io.ConfigFlags & ImGuiConfigFlags_DockingEnable);

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetWorkPos());
	ImGui::SetNextWindowSize(viewport->GetWorkSize());
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	auto isOpen = IGui::beginView();
	ImGui::PopStyleVar(3);

	return isOpen;
}

void MainDockspace::renderView()
{
	bool bHasProject = Editor::EDITOR->hasProject();
	if (bHasProject)
	{
		ImGui::DockSpace(ImGui::GetID(this->mId.c_str()), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}
	else
	{
		ImGui::Text("You have not opened a project.");
		if (ImGui::Button("Open Project"))
		{
			auto gui = Editor::EDITOR->openNewGui<gui::modal::OpenAsset>("Open Project");
			gui->addAssetType(asset::Project::StaticType());
			gui->setCallback(this->mOnProjectOpenedOrCreated);
		}
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Project", "", false, true))
			{
				auto gui = Editor::EDITOR->openNewGui<gui::modal::NewAsset>("New Project");
				gui->setAssetType(asset::Project::StaticType());
				gui->setCallback(this->mOnProjectOpenedOrCreated);
			}
			if (ImGui::MenuItem("Open Project", "", false, true))
			{
				auto gui = Editor::EDITOR->openNewGui<gui::modal::OpenAsset>("Open Project");
				gui->addAssetType(asset::Project::StaticType());
				gui->setCallback(this->mOnProjectOpenedOrCreated);
			}
			if (ImGui::MenuItem("Project Settings", "", false, bHasProject)) Editor::EDITOR->openProjectSettings();
			ImGui::Separator();
			if (ImGui::MenuItem("New Asset", "", false, bHasProject))
			{
				std::shared_ptr<gui::modal::NewAsset> gui = Editor::EDITOR->openNewGui<gui::modal::NewAsset>("New Asset");
				gui->setCallback(this->mOnAssetCreated);
			}
			if (ImGui::MenuItem("Build", "", false, bHasProject && !Editor::EDITOR->isBuildingAssets()))
			{
				this->mbIsBuildingAssets = true;
				Editor::EDITOR->buildAllAssets();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Settings", "", false, bHasProject)) Editor::EDITOR->openSettings();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Windows"))
		{
			if (ImGui::MenuItem("Asset Browser", "", false, bHasProject && this->mpAssetBrowser.expired()))
			{
				this->mpAssetBrowser = Editor::EDITOR->openNewGui<gui::AssetBrowser>("Asset Browser");
			}
			if (ImGui::MenuItem("Log (Editor)", "", false, bHasProject && this->mpEditorLog.expired()))
			{
				this->mpEditorLog = Editor::EDITOR->openNewGui<gui::Log>("Log (Editor)");
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}
