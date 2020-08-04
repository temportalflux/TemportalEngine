#include "gui/asset/EditorRenderPass.hpp"

#include "asset/AssetManager.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/RenderPassAsset.hpp"
#include "gui/widget/FieldAsset.hpp"
#include "gui/widget/FieldNumber.hpp"
#include "gui/widget/List.hpp"
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
	this->mClearColor = asset->getClearColorValue();
	this->mClearDepthStencil = asset->getDepthStencilClearValues();
	this->mRenderArea = asset->getRenderArea();
	this->mPipelines = asset->getPipelineRefs();

	this->mAllPipelinePaths = asset::AssetManager::get()->getAssetList(asset::Pipeline::StaticType());

}

void EditorRenderPass::renderContent()
{
	AssetEditor::renderContent();

	if (ImGui::TreeNode("Clear Values"))
	{

		if (gui::Optional<math::Vector4>::Inline(this->mClearColor, "Clear Color", true, [](math::Vector4 &clearColor) -> bool {
			ImGui::SameLine();
			return ImGui::ColorEdit4("###color", clearColor.data(), ImGuiColorEditFlags_None);
		}))
		{
			this->markAssetDirty(1);
		}

		if (gui::Optional<std::pair<f32, ui32>>::Inline(
			this->mClearDepthStencil, "Clear Depth/Stencil", true,
			[](std::pair<f32, ui32>& depthStencil) -> bool {
			bool bChangedDepth = gui::FieldNumber<f32, 1>::InlineSingle("Clear Depth", depthStencil.first);
			bool bChangedStencil = gui::FieldNumber<ui32, 1>::InlineSingle("Clear Stencil", depthStencil.second);
			return bChangedDepth || bChangedStencil;
		}
		))
		{
			this->markAssetDirty(1);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Render Area"))
	{
		if (gui::FieldNumber<f32, 2>::InlineVector("Offset", this->mRenderArea.offset))
		{
			this->markAssetDirty(1);
		}
		if (gui::FieldNumber<f32, 2>::InlineVector("Size", this->mRenderArea.size))
		{
			this->markAssetDirty(1);
		}
		ImGui::TreePop();
	}

	if (
		gui::List<asset::TypedAssetPath<asset::Pipeline>>::Inline(
			"Pipelines", "Pipelines", /*values collapse*/ false,
			this->mPipelines,
			[&](uIndex const& idx, asset::TypedAssetPath<asset::Pipeline> &item) -> bool {
				return gui::FieldAsset::Inline("item", item.path(), this->mAllPipelinePaths, asset::Pipeline::StaticType());
			}
		)
	)
	{
		this->markAssetDirty(1);
	}
}

void EditorRenderPass::saveAsset()
{
	this->get<asset::RenderPass>()
		->setClearColor(this->mClearColor)
		.setDepthStencilClearValues(this->mClearDepthStencil)
		.setRenderArea(this->mRenderArea)
		.setPipelineRefs(this->mPipelines);
	AssetEditor::saveAsset();
}
