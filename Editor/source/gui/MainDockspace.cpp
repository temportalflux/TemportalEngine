#include "gui/MainDockspace.hpp"

#include "Editor.hpp"
#include "graphics/ImGuiRenderer.hpp"

#include <imgui.h>

using namespace gui;

static const char* MODAL_ID_NEW_PROJECT = "new_project";

MainDockspace::MainDockspace(std::string id, std::string title)
	: IGui(title), mId(id)
	, mModalNewProject("New Project")
	, mModalOpenProject("Open Project")
	, mModalNewAsset("New Asset")
{
	this->mAssetBrowser = gui::AssetBrowser("Asset Browser");
	this->mLogEditor = gui::Log("Log (Editor)");

	auto onProjectAsset = [&](asset::AssetPtrStrong asset)
	{
		Editor::EDITOR->setProject(asset);
	};
	this->mModalNewProject.setAssetType(AssetType_Project);
	this->mModalNewProject.setCallback(onProjectAsset);
	this->mModalOpenProject.setCallback(onProjectAsset);

	this->mModalNewAsset.setCallback([](asset::AssetPtrStrong asset) {
		Editor::EDITOR->openAssetEditor(asset);
	});
}

void MainDockspace::onAddedToRenderer(graphics::ImGuiRenderer *pRenderer)
{
	IGui::onAddedToRenderer(pRenderer);
	pRenderer->addGui(&this->mAssetBrowser);
	pRenderer->addGui(&this->mLogEditor);
}

void MainDockspace::onRemovedFromRenderer(graphics::ImGuiRenderer *pRenderer)
{
	IGui::onRemovedFromRenderer(pRenderer);
	pRenderer->removeGui(&this->mAssetBrowser);
	pRenderer->removeGui(&this->mLogEditor);
}

void MainDockspace::makeGui()
{
	IGui::makeGui();
	this->mModalNewProject.draw();
	this->mModalOpenProject.draw();
	this->mModalNewAsset.draw();
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
			this->mModalOpenProject.open();
		}
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Project", "", false, true)) this->mModalNewProject.open();
			if (ImGui::MenuItem("Open Project", "", false, true)) this->mModalOpenProject.open();
			ImGui::Separator();
			if (ImGui::MenuItem("New Asset", "", false, bHasProject)) this->mModalNewAsset.open();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Windows"))
		{
			if (ImGui::MenuItem("Asset Browser", "", this->mAssetBrowser.isOpen(), bHasProject)) this->mAssetBrowser.openOrFocus();
			if (ImGui::MenuItem("Log (Editor)", "", this->mLogEditor.isOpen(), bHasProject)) this->mLogEditor.openOrFocus();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}
