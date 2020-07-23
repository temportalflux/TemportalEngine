#include "gui/asset/EditorTexture.hpp"

#include "asset/AssetManager.hpp"
#include "asset/Texture.hpp"
#include "gui/modal/PathModal.hpp"
#include "memory/MemoryChunk.hpp"
#include "Editor.hpp"

#include "math/Vector.hpp"

#include <imgui.h>

using namespace gui;

std::shared_ptr<AssetEditor> EditorTexture::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorTexture>();
}

void EditorTexture::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
}

void EditorTexture::renderMenuBarItems()
{
	AssetEditor::renderMenuBarItems();
	if (ImGui::MenuItem("Import", "", false, this->mModalImport.expired()))
	{
		this->mModalImport = Editor::EDITOR->openNewGui<gui::modal::PathModal>("Import Image");
		this->mModalImport.lock()->setCallback(std::bind(&EditorTexture::onImportConfirmed, this, std::placeholders::_1));
	}
}

void EditorTexture::makeGui()
{
	AssetEditor::makeGui();
}

void EditorTexture::onImportConfirmed(std::filesystem::path path)
{
	auto asset = this->get<asset::Texture>();
	asset->setSourcePath(std::filesystem::relative(path, asset->getPath().parent_path()));
	this->saveAsset();
}
