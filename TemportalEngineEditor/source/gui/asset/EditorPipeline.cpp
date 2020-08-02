#include "gui/asset/EditorPipeline.hpp"

#include "asset/PipelineAsset.hpp"
#include "gui/widget/Optional.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

// Macros for the bits each field of the project asset correspond to in the dirty flags

std::shared_ptr<AssetEditor> EditorPipeline::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorPipeline>();
}

void EditorPipeline::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::Pipeline>();

}

void EditorPipeline::renderContent()
{
	AssetEditor::renderContent();

	auto asset = this->get<asset::Pipeline>();

}

void EditorPipeline::saveAsset()
{
	auto asset = this->get<asset::Pipeline>();
	AssetEditor::saveAsset();
}
