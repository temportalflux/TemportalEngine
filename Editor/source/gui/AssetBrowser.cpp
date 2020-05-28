#include "gui/AssetBrowser.hpp"

#include "logging/Logger.hpp"

#include <imgui.h>

using namespace gui;

AssetBrowser::AssetBrowser(std::string title) : IGui(title)
{
}

i32 AssetBrowser::getFlags() const
{
	return ImGuiWindowFlags_None;
}

void AssetBrowser::renderView()
{
	ImGui::Text("This will be the asset browser");
}
