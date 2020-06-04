#include "gui/asset/EditorShader.hpp"

#include "asset/Shader.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

std::shared_ptr<AssetEditor> EditorShader::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorShader>();
}

void EditorShader::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::Shader>();

}

void EditorShader::renderView()
{
	AssetEditor::renderView();

	auto asset = this->get<asset::Shader>();

}

void EditorShader::saveAsset()
{
	auto asset = this->get<asset::Shader>();
	AssetEditor::saveAsset();
}
