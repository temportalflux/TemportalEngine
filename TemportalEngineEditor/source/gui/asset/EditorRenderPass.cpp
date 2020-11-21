#include "gui/asset/EditorRenderPass.hpp"

#include "asset/AssetManager.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/RenderPassAsset.hpp"
#include "gui/widget/Combo.hpp"
#include "gui/widget/FieldAsset.hpp"
#include "gui/widget/FieldNumber.hpp"
#include "gui/widget/FieldText.hpp"
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
	this->mPhases = asset->getPhases();
	this->mPhaseDependencies = asset->getPhaseDependencies();

	this->mAllPipelinePaths = asset::AssetManager::get()->getAssetList(asset::Pipeline::StaticType());
	this->mPhaseNames = this->makePhaseNames();

	this->mfRenderPhaseName = std::bind(&EditorRenderPass::renderPhaseName, this, std::placeholders::_1, std::placeholders::_2);
	this->mfRenderDependencyKey = std::bind(&EditorRenderPass::renderPhaseDependencyKey, this, std::placeholders::_1, std::placeholders::_2);
	this->mfRenderDependency = std::bind(&EditorRenderPass::renderPhaseDependency, this, std::placeholders::_1, std::placeholders::_2);
	this->mfRenderDependencyItemName = std::bind(&EditorRenderPass::renderPhaseDependencyItemPhaseName, this, std::placeholders::_1);
}

bool renderPhase(uIndex const &idx, graphics::RPPhase &phase);
bool renderPhaseAttachmentElement(uIndex const &idx, graphics::RPPhase::Attachment &attachment);
bool renderPhaseAttachment(graphics::RPPhase::Attachment &attachment);

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

	if (gui::List<graphics::RPPhase>::Inline("Phases", "Phases", true, this->mPhases, &renderPhase, this->mfRenderPhaseName))
	{
		this->markAssetDirty(1);
		if (this->mPhaseNames.size() != this->mPhases.size()) this->mPhaseNames = this->makePhaseNames();
	}

	if (gui::List<graphics::RPDependency>::Inline("deps", "Phase Dependencies", true, this->mPhaseDependencies, this->mfRenderDependency, this->mfRenderDependencyKey))
	{
		this->markAssetDirty(1);
	}

}

bool EditorRenderPass::renderPhaseName(uIndex const &idx, graphics::RPPhase &phase)
{
	if (gui::FieldText<16>::Inline("###name", phase.name))
	{
		this->mPhaseNames[idx] = phase.name;
		return true;
	}
	return false;
}

bool renderPhase(uIndex const &idx, graphics::RPPhase &phase)
{
	bool bChanged = false;
	if (gui::List<graphics::RPPhase::Attachment>::Inline("color", "Color Attachments", true, phase.colorAttachments, &renderPhaseAttachmentElement)) bChanged = true;
	if (ImGui::TreeNode("Depth Attachment"))
	{
		if (gui::Optional<graphics::RPPhase::Attachment>::Inline(phase.depthAttachment, "Has Attachment", true, &renderPhaseAttachment)) bChanged = true;
		ImGui::TreePop();
	}
	return bChanged;
}

bool renderPhaseAttachmentElement(uIndex const &idx, graphics::RPPhase::Attachment &attachment)
{
	return renderPhaseAttachment(attachment);
}

bool renderPhaseAttachment(graphics::RPPhase::Attachment &attachment)
{
	bool bChanged = false;

	if (gui::Combo<graphics::ImageFormatReferenceType::Enum>::Inline(
		"Format Type", graphics::ImageFormatReferenceType::ALL,
		attachment.formatType,
		graphics::ImageFormatReferenceType::to_string,
		[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
	)) bChanged = true;

	if (gui::Combo<graphics::SampleCount::Enum>::Inline(
		"Samples", graphics::SampleCount::ALL,
		attachment.samples,
		graphics::SampleCount::to_string,
		[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
	)) bChanged = true;

	if (ImGui::TreeNode("General Operations"))
	{
		if (gui::Combo<graphics::AttachmentLoadOp::Enum>::Inline(
			"Load", graphics::AttachmentLoadOp::ALL,
			attachment.generalLoadOp,
			graphics::AttachmentLoadOp::to_string,
			[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
		)) bChanged = true;

		if (gui::Combo<graphics::AttachmentStoreOp::Enum>::Inline(
			"Store", graphics::AttachmentStoreOp::ALL,
			attachment.generalStoreOp,
			graphics::AttachmentStoreOp::to_string,
			[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
		)) bChanged = true;

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Stencil Operations"))
	{
		if (gui::Combo<graphics::AttachmentLoadOp::Enum>::Inline(
			"Load", graphics::AttachmentLoadOp::ALL,
			attachment.stencilLoadOp,
			graphics::AttachmentLoadOp::to_string,
			[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
		)) bChanged = true;

		if (gui::Combo<graphics::AttachmentStoreOp::Enum>::Inline(
			"Store", graphics::AttachmentStoreOp::ALL,
			attachment.stencilStoreOp,
			graphics::AttachmentStoreOp::to_string,
			[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
		)) bChanged = true;

		ImGui::TreePop();
	}

	return bChanged;
}

bool EditorRenderPass::renderPhaseDependencyKey(uIndex const &idx, graphics::RPDependency &dependency)
{
	ImGui::Text(
		"%s -> %s",
		dependency.dependee.phaseIndex ? (
			this->mPhaseNames.size() > *dependency.dependee.phaseIndex
			? this->mPhaseNames[*dependency.dependee.phaseIndex].c_str()
			: "(no name available)"
		) : "none",
		dependency.depender.phaseIndex ? (
			this->mPhaseNames.size() > *dependency.depender.phaseIndex
			? this->mPhaseNames[*dependency.depender.phaseIndex].c_str()
			: "(no name available)"
		) : "none"
	);
	return false;
}

bool EditorRenderPass::renderPhaseDependency(uIndex const &idx, graphics::RPDependency &dependency)
{
	bool bChanged = false;
	if (ImGui::TreeNode("Dependee"))
	{
		if (this->renderPhaseDependencyItem(dependency.dependee)) bChanged = true;
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Depender"))
	{
		if (this->renderPhaseDependencyItem(dependency.depender)) bChanged = true;
		ImGui::TreePop();
	}
	return bChanged;
}

bool EditorRenderPass::renderPhaseDependencyItem(graphics::RPDependency::Item &item)
{
	bool bChanged = false;
	if (gui::Optional<uIndex>::Inline(item.phaseIndex, "Phase", true, this->mfRenderDependencyItemName)) bChanged = true;

	/*
	auto stageMaskSet = item.stageMask.toSet(graphics::PipelineStage::ALL);
	std::string stageMaskPreviewStr = std::to_string(stageMaskSet.size()) + " Stages";
	if (gui::Combo<graphics::PipelineStage::Enum>::InlineMulti(
		"Stage Mask", graphics::PipelineStage::ALL,
		stageMaskSet, stageMaskPreviewStr,
		graphics::PipelineStage::to_string,
		[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
	))
	{
		bChanged = true;
		item.stageMask = utility::Flags<graphics::PipelineStage::Enum>(stageMaskSet);
	}

	auto accessMaskSet = item.accessMask.toSet(graphics::Access::ALL);
	std::string accessMaskPreviewStr = std::to_string(accessMaskSet.size()) + " Flags";
	if (gui::Combo<graphics::Access::Enum>::InlineMulti(
		"Access Mask", graphics::Access::ALL,
		accessMaskSet, accessMaskPreviewStr,
		graphics::Access::to_string,
		[](auto enumVal) { ImGui::PushID((ui32)enumVal); }
	))
	{
		bChanged = true;
		item.accessMask = utility::Flags<graphics::Access::Enum>(accessMaskSet);
	}
	//*/

	return bChanged;
}

bool EditorRenderPass::renderPhaseDependencyItemPhaseName(uIndex &phaseIdx)
{
	ImGui::SameLine();
	if (this->mPhaseNames.size() <= phaseIdx)
	{
		ImGui::Text("No phase available");
		return false;
	}
	std::string selected = this->mPhaseNames[phaseIdx];
	if (gui::Combo<std::string>::Inline(
		"###phaseIndex", this->mPhaseNames, selected,
		[](auto str) { return str; },
		[](auto str) { ImGui::PushID(str.c_str()); }
	))
	{
		phaseIdx = std::distance(this->mPhaseNames.begin(), std::find(this->mPhaseNames.begin(), this->mPhaseNames.end(), selected));
		return true;
	}
	return false;
}

std::vector<std::string> EditorRenderPass::makePhaseNames()
{
	auto names = std::vector<std::string>();
	std::transform(this->mPhases.begin(), this->mPhases.end(), std::back_inserter(names), [](auto phase) { return phase.name; });
	return names;
}

void EditorRenderPass::saveAsset()
{
	this->get<asset::RenderPass>()
		->setClearColor(this->mClearColor)
		.setDepthStencilClearValues(this->mClearDepthStencil)
		.setRenderArea(this->mRenderArea)
		.setPipelineRefs(this->mPipelines)
		.setPhases(this->mPhases)
		.setPhaseDependencies(this->mPhaseDependencies);
	AssetEditor::saveAsset();
}
