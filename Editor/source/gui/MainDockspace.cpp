#include "gui/MainDockspace.hpp"

#include <imgui.h>

using namespace gui;

MainDockspace::MainDockspace()
	: mbIsOpen(true)
{
}

ui32 MainDockspace::getId() const
{
	return ImGui::GetID("Editor::MainDockspace");
}

i32 MainDockspace::getFlags() const
{
	return
		ImGuiWindowFlags_MenuBar
		| (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)
		| (ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
}

bool MainDockspace::isOpen() const
{
	return this->mbIsOpen;
}

void MainDockspace::makeGui()
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
	ImGui::Begin("Main Dockspace", &this->mbIsOpen, getFlags());
	ImGui::PopStyleVar(3);

	ImGui::DockSpace(this->getId(), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	if (ImGui::BeginMenuBar())
	{
		ImGui::EndMenuBar();
	}

	ImGui::End();

	ImGui::Begin("Hello, world!");
	ImGui::Text("This is some useful text.");
	ImGui::Button("Button");
	ImGui::End();

	ImGui::Begin("Window 2");
	ImGui::Text("This is some meaningless text.");
	ImGui::End();
}
