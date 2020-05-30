#include "gui/modal/ModalNewProject.hpp"

#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

ModalNewProject::ModalNewProject() : Modal("New Project")
{
	this->mInputDirectory.fill('\0');
	this->mInputName.fill('\0');
}

void ModalNewProject::drawContents()
{
	ImGui::InputText("Directory", this->mInputDirectory.data(), this->mInputDirectory.size());
	ImGui::InputText("Project Name", this->mInputName.data(), this->mInputName.size());
	if (ImGui::Button("Create"))
	{
		this->submit();
	}
}

void ModalNewProject::submit()
{
	auto directory = utility::createStringFromFixedArray(this->mInputDirectory);
	auto name = utility::createStringFromFixedArray(this->mInputName);
	asset::AssetManager::createProject(directory + "\\" + name, name);
	this->close();
}

void ModalNewProject::reset()
{
	Modal::reset();
	this->mInputDirectory.fill('\0');
	this->mInputName.fill('\0');
}
