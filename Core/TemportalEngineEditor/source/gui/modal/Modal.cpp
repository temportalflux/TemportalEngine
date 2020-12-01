#include "gui/modal/Modal.hpp"

#include <imgui.h>

using namespace gui::modal;

Modal::Modal(std::string title) : IGui(title), bOpenOnNextFrame(false)
{
}

void Modal::open()
{
	IGui::open();
	this->bOpenOnNextFrame = true;
}

void Modal::makeGui()
{
	if (this->bOpenOnNextFrame)
	{
		ImGui::OpenPopup(this->getId().c_str());
		this->bOpenOnNextFrame = false;
	}
	if (ImGui::BeginPopupModal(this->getId().c_str(), 0, ImGuiWindowFlags_None))
	{
		this->drawContents();
		ImGui::EndPopup();
	}
	if (this->shouldReleaseGui())
	{
		this->removeGui();
	}
}
