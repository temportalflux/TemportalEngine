#include "gui/asset/EditorSettings.hpp"

#include "Editor.hpp"
#include "asset/Settings.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

#define Bit_OutputDirectory 1 << 0

std::shared_ptr<AssetEditor> EditorSettings::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorSettings>();
}

void EditorSettings::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
	auto asset = this->get<asset::Settings>();
	memcpy(this->mInput_OutputDirectory.data(), asset->getOutputDirectory().data(), asset->getOutputDirectory().length());
}

std::string EditorSettings::getOutputDirectory() const
{
	return utility::createStringFromFixedArray(this->mInput_OutputDirectory);
}

void EditorSettings::renderContent()
{
	AssetEditor::renderContent();
	auto asset = this->get<asset::Settings>();
	if (ImGui::InputText("Output Directory", this->mInput_OutputDirectory.data(), this->mInput_OutputDirectory.size()))
	{
		this->markAssetDirty(Bit_OutputDirectory, this->getOutputDirectory() != asset->getOutputDirectory());
	}
	ImGui::Text("Output Directory (Absolute): ");
	ImGui::Text(std::filesystem::absolute(Editor::EDITOR->getProject()->getAbsoluteDirectoryPath() / this->getOutputDirectory()).string().c_str());
}

void EditorSettings::saveAsset()
{
	auto asset = this->get<asset::Settings>();
	asset->setOutputDirectory(this->getOutputDirectory());
	AssetEditor::saveAsset();
}
