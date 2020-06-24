#include "gui/asset/EditorImage.hpp"

#include "asset/AssetManager.hpp"
#include "asset/Texture.hpp"
#include "gui/modal/PathModal.hpp"
#include "memory/MemoryChunk.hpp"

#include "math/Vector.hpp"

#include <imgui.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace gui;

std::shared_ptr<AssetEditor> EditorImage::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	auto editor = mem->make_shared<EditorImage>();
	editor->mModalImport = mem->make_shared<gui::modal::PathModal>("Import Image");
	editor->mModalImport->setCallback(std::bind(&EditorImage::onImportConfirmed, editor.get(), std::placeholders::_1));
	return editor;
}

void EditorImage::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
}

void EditorImage::renderMenuBarItems()
{
	AssetEditor::renderMenuBarItems();
	if (ImGui::MenuItem("Import", "", false, true)) this->mModalImport->open();
}

void EditorImage::makeGui()
{
	IGui::makeGui();
	this->mModalImport->draw();
}

void EditorImage::onImportConfirmed(std::filesystem::path path)
{
	auto asset = this->get<asset::Texture>();
	asset->setSourcePath(std::filesystem::relative(path, asset->getPath().parent_path()));
	this->saveAsset();
}

void EditorImage::compileAsset()
{
	static i32 LOAD_MODE = STBI_rgb_alpha;
	static ui32 LOAD_MODE_SIZE = 4; // 4 bytes per pixel

	auto asset = this->get<asset::Texture>();
	auto srcPathStr = asset->getAbsoluteSourcePath().string();
	auto srcDimensions = math::Vector2Int();
	i32 srcChannels;
	
	ui8* data = stbi_load(srcPathStr.c_str(), &srcDimensions.x(), &srcDimensions.y(), &srcChannels, STBI_rgb_alpha);
	std::vector<ui8> imageData(srcDimensions.x() * srcDimensions.y() * LOAD_MODE_SIZE);
	memcpy(imageData.data(), data, imageData.capacity());
	stbi_image_free(data);

	asset->setSourceBinary(imageData, { (ui32)srcDimensions.x(), (ui32)srcDimensions.y() });

	AssetEditor::compileAsset();
}
