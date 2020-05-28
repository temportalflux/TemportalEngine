#include "gui/MainDockspace.hpp"

#include "graphics/ImGuiRenderer.hpp"

#include <imgui.h>

using namespace gui;

MainDockspace::MainDockspace(std::string id, std::string title) : IGui(title), mId(id)
{
	this->mAssetBrowser = gui::AssetBrowser("Asset Browser");
	this->mLogEditor = gui::Log("Log (Editor)");
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
	ImGui::DockSpace(ImGui::GetID(this->mId.c_str()), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Windows"))
		{
			if (ImGui::MenuItem("Asset Browser", "", this->mAssetBrowser.isOpen(), true)) this->mAssetBrowser.openOrFocus();
			if (ImGui::MenuItem("Log (Editor)", "", this->mLogEditor.isOpen(), true)) this->mLogEditor.openOrFocus();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}
