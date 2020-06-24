#include "gui/modal/Modal.hpp"

#include <imgui.h>

using namespace gui::modal;

Modal::Modal(char const *title) : mTitle(title), mbShouldBeOpen(false)
{
}

void Modal::open()
{
	this->mbShouldBeOpen = true;
}

void Modal::draw()
{
	if (this->mbShouldBeOpen) ImGui::OpenPopup(this->mTitle);
	if (ImGui::BeginPopupModal(this->mTitle, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		this->drawContents();
		ImGui::EndPopup();
	}
}

void Modal::close()
{
	ImGui::CloseCurrentPopup();
	this->reset();
}

void Modal::reset()
{
	this->mbShouldBeOpen = false;
}
