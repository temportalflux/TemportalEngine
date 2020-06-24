#include "gui/modal/PathModal.hpp"

#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

PathModal::PathModal(char const *title) : Modal(title)
{
	this->mInputPath.fill('\0');
}

void PathModal::setDefaultPath(std::filesystem::path path)
{
	memcpy(this->mInputPath.data(), path.string().data(), path.string().length());
}

void PathModal::setCallback(PathSelectedCallback callback)
{
	this->mOnPathSelected = callback;
}

void PathModal::drawContents()
{
	ImGui::InputText("Path", this->mInputPath.data(), this->mInputPath.size());
	if (ImGui::Button("Open"))
	{
		this->submit();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		this->close();
	}
}

void PathModal::submit()
{
	this->mOnPathSelected(utility::createStringFromFixedArray(this->mInputPath));
	this->close();
}

void PathModal::reset()
{
	Modal::reset();
	this->mInputPath.fill('\0');
}
