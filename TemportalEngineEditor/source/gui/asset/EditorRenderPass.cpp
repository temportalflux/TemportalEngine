#include "gui/asset/EditorRenderPass.hpp"

#include "asset/RenderPassAsset.hpp"
#include "gui/widget/Optional.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

// Macros for the bits each field of the project asset correspond to in the dirty flags

std::shared_ptr<AssetEditor> EditorRenderPass::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorRenderPass>();
}

void EditorRenderPass::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::RenderPass>();

}

void EditorRenderPass::renderContent()
{
	AssetEditor::renderContent();

	auto asset = this->get<asset::RenderPass>();

}

void EditorRenderPass::saveAsset()
{
	auto asset = this->get<asset::RenderPass>();
	AssetEditor::saveAsset();
}
