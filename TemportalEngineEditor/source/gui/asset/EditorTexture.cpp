#include "gui/asset/EditorTexture.hpp"

#include "asset/AssetManager.hpp"
#include "asset/Texture.hpp"
#include "gui/modal/PathModal.hpp"
#include "memory/MemoryChunk.hpp"

#include "math/Vector.hpp"

#include <imgui.h>

using namespace gui;

std::shared_ptr<AssetEditor> EditorTexture::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	auto editor = mem->make_shared<EditorTexture>();
	editor->mModalImport = mem->make_shared<gui::modal::PathModal>("Import Image");
	editor->mModalImport->setCallback(std::bind(&EditorTexture::onImportConfirmed, editor.get(), std::placeholders::_1));
	return editor;
}

void EditorTexture::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
}

void EditorTexture::renderMenuBarItems()
{
	AssetEditor::renderMenuBarItems();
	if (ImGui::MenuItem("Import", "", false, true)) this->mModalImport->open();
}

void EditorTexture::makeGui()
{
	AssetEditor::makeGui();
	this->mModalImport->draw();
}

void EditorTexture::onImportConfirmed(std::filesystem::path path)
{
	auto asset = this->get<asset::Texture>();
	asset->setSourcePath(std::filesystem::relative(path, asset->getPath().parent_path()));
	this->saveAsset();
}
