#include "gui/modal/OpenProject.hpp"

#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui::modal;

OpenProject::OpenProject() : Modal("Open Project")
{
	this->mInputPath.fill('\0');
}

void OpenProject::drawContents()
{
	ImGui::InputText("Directory", this->mInputPath.data(), this->mInputPath.size());
	if (ImGui::Button("Open"))
	{
		this->submit();
	}
}

void OpenProject::submit()
{
	auto filePath = utility::createStringFromFixedArray(this->mInputPath);
	asset::AssetManager::openProject(filePath);
	this->close();
}

void OpenProject::reset()
{
	Modal::reset();
	this->mInputPath.fill('\0');
}
