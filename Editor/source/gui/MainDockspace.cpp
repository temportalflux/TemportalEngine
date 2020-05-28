#include "gui/MainDockspace.hpp"

#include "graphics/ImGuiRenderer.hpp"

#include <imgui.h>

using namespace gui;

MainDockspace::MainDockspace(std::string id, std::string title) : IGui(title), mId(id)
{
	this->mAssetBrowser = gui::AssetBrowser("Asset Browser");
}

void MainDockspace::onAddedToRenderer(graphics::ImGuiRenderer *pRenderer)
{
	pRenderer->addGui(&this->mAssetBrowser);
}

void MainDockspace::onRemovedFromRenderer(graphics::ImGuiRenderer *pRenderer)
{
	pRenderer->removeGui(&this->mAssetBrowser);
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
			if (ImGui::MenuItem("Asset Browser", "", this->mAssetBrowser.isOpen(), true))
			{
				this->mAssetBrowser.openOrFocus();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	/*
	ImGui::Begin("Hello, world!");
	ImGui::Text("This is some useful text.");
	ImGui::Button("Button");
	ImGui::End();

	ImGui::Begin("Window 2");
	ImGui::Text("This is some meaningless text.");
	ImGui::End();
	 */
}
