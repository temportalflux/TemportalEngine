#include "gui/modal/Modal.hpp"

#include <imgui.h>

using namespace gui::modal;

Modal::Modal(std::string title) : IGui(title)
{
}

void Modal::makeGui()
{
	if (this->isOpen()) ImGui::OpenPopup(this->getId().c_str());
	if (ImGui::BeginPopupModal(this->getId().c_str(), 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		this->drawContents();
		ImGui::EndPopup();
	}
	if (this->shouldReleaseGui())
	{
		this->removeGui();
	}
}
