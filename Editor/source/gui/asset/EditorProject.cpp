#include "gui/asset/EditorProject.hpp"

#include "asset/Project.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

// Macros for the bits each field of the project asset correspond to in the dirty flags
#define Bit_Name 1 << 0
#define Bit_Version 1 << 1

std::shared_ptr<AssetEditor> EditorProject::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorProject>();
}

void EditorProject::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
	auto asset = this->get<asset::Project>();
	memcpy(this->mInputName.data(), asset->getName().data(), asset->getName().length());
}

std::string EditorProject::getInputName() const
{
	return utility::createStringFromFixedArray(this->mInputName);
}

void EditorProject::renderView()
{
	AssetEditor::renderView();

	auto asset = this->get<asset::Project>();
	
	ImGui::Text("Project Editor");
	if (ImGui::InputText("Name", this->mInputName.data(), this->mInputName.size()))
	{
		this->markAssetDirty(Bit_Name, this->getInputName() != asset->getName());
	}
}

void EditorProject::saveAsset()
{
	auto asset = this->get<asset::Project>();
	asset->setName(this->getInputName());
	AssetEditor::saveAsset();
}
