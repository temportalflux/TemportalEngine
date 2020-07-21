#include "gui/modal/ModalFilePicker.hpp"

#include <imgui.h>

using namespace gui::modal;

FilePicker::FilePicker(char const *title) : Modal(title)
{
}

void FilePicker::drawContents()
{
	if (ImGui::Button("Open"))
	{
		this->submit();
	}
}

void FilePicker::submit()
{
	this->close();
}

void FilePicker::reset()
{
	Modal::reset();
}
