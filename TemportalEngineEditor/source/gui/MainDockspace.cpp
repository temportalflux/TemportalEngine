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
	auto memory = Editor::EDITOR->getMemoryGui();

	this->mAssetBrowser = memory->make_shared<gui::AssetBrowser>("Asset Browser");
	this->mLogEditor = memory->make_shared<gui::Log>("Log (Editor)");

	this->mModalNewProject = memory->make_shared<gui::modal::NewAsset>("New Project");
	this->mModalNewProject->setAssetType(AssetType_Project);
	this->mModalOpenProject = memory->make_shared<gui::modal::OpenAsset>("Open Project");
	this->mModalNewAsset = memory->make_shared<gui::modal::NewAsset>("New Asset");

	auto onProjectAsset = [&](asset::AssetPtrStrong asset)
	{
		Editor::EDITOR->setProject(asset);
	};
	this->mModalNewProject->setCallback(onProjectAsset);
	this->mModalOpenProject->setCallback(onProjectAsset);
	this->mModalNewAsset->setCallback([](asset::AssetPtrStrong asset) {
		Editor::EDITOR->openAssetEditor(asset);
	});
}

void MainDockspace::onAddedToRenderer(graphics::ImGuiRenderer *pRenderer)
{
	IGui::onAddedToRenderer(pRenderer);
	pRenderer->addGui("AssetBrowser", this->mAssetBrowser);
	pRenderer->addGui("EditorLog", this->mLogEditor);
}

void MainDockspace::onRemovedFromRenderer(graphics::ImGuiRenderer *pRenderer)
{
	IGui::onRemovedFromRenderer(pRenderer);
	pRenderer->removeGui("AssetBrowser");
	pRenderer->removeGui("EditorLog");
}

void MainDockspace::makeGui()
{
	IGui::makeGui();
	this->mModalNewProject->draw();
	this->mModalOpenProject->draw();
	this->mModalNewAsset->draw();

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

void openProject(std::shared_ptr<gui::modal::OpenAsset> modal)
{
	modal->setDefaultPath(std::filesystem::absolute("../../DemoGame/DemoGame.te-project"));
	modal->open();
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
			openProject(this->mModalOpenProject);
		}
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Project", "", false, true)) this->mModalNewProject->open();
			if (ImGui::MenuItem("Open Project", "", false, true)) openProject(this->mModalOpenProject);
			if (ImGui::MenuItem("Project Settings", "", false, bHasProject)) Editor::EDITOR->openProjectSettings();
			ImGui::Separator();
			if (ImGui::MenuItem("New Asset", "", false, bHasProject)) this->mModalNewAsset->open();
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
			if (ImGui::MenuItem("Asset Browser", "", this->mAssetBrowser->isOpen(), bHasProject)) this->mAssetBrowser->openOrFocus();
			if (ImGui::MenuItem("Log (Editor)", "", this->mLogEditor->isOpen(), bHasProject)) this->mLogEditor->openOrFocus();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}
