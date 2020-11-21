#include "gui/asset/EditorPipeline.hpp"

#include "asset/AssetManager.hpp"
#include "asset/Shader.hpp"
#include "gui/widget/Combo.hpp"
#include "gui/widget/FieldAsset.hpp"
#include "gui/widget/FieldNumber.hpp"
#include "gui/widget/FieldText.hpp"
#include "gui/widget/List.hpp"
#include "gui/widget/Optional.hpp"
#include "memory/MemoryChunk.hpp"

#include "property/Property.hpp"

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
	this->mDescriptorGroups = asset->getDescriptorGroups();
}

bool renderDescriptorGroup(uIndex const& idx, asset::Pipeline::DescriptorGroup &group);
bool renderDescriptor(uIndex const& idx, asset::Pipeline::Descriptor &descriptor);

void EditorPipeline::renderContent()
{
	AssetEditor::renderContent();
	auto asset = this->get<asset::Pipeline>();

	if (properties::renderProperty("Viewport", asset->REF_PROP(Viewport), asset->DEFAULT_PROP(Viewport)))
	{
		this->markAssetDirty(1);
	}

	if (properties::renderProperty("Scissor", asset->REF_PROP(Scissor), asset->DEFAULT_PROP(Scissor)))
	{
		this->markAssetDirty(1);
	}

	if (properties::renderProperty("Blend Mode", asset->REF_PROP(BlendMode), asset->DEFAULT_PROP(BlendMode)))
	{
		this->markAssetDirty(1);
	}

	if (properties::renderProperty("Front Face", asset->REF_PROP(FrontFace), asset->DEFAULT_PROP(FrontFace)))
	{
		this->markAssetDirty(1);
	}

	if (properties::renderProperty("Topology", asset->REF_PROP(Topology), asset->DEFAULT_PROP(Topology)))
	{
		this->markAssetDirty(1);
	}

	if (properties::renderProperty("Line Width", asset->REF_PROP(LineWidth), asset->DEFAULT_PROP(LineWidth)))
	{
		this->markAssetDirty(1);
	}

	if (properties::renderProperty("Vertex Shader", asset->REF_PROP(VertexShader), asset->DEFAULT_PROP(VertexShader)))
	{
		this->markAssetDirty(1);
	}

	if (properties::renderProperty("Fragment Shader", asset->REF_PROP(FragmentShader), asset->DEFAULT_PROP(FragmentShader)))
	{
		this->markAssetDirty(1);
	}

	if (gui::List<asset::Pipeline::DescriptorGroup>::Inline("descriptorGroups", "Descriptor Groups", false, this->mDescriptorGroups, &renderDescriptorGroup))
	{
		this->markAssetDirty(1);
	}

}

bool renderDescriptorGroup(uIndex const& idx, asset::Pipeline::DescriptorGroup &group)
{
	return gui::List<asset::Pipeline::Descriptor>::Inline("descriptors", "Descriptors", false, group.descriptors, &renderDescriptor);
}

bool renderDescriptor(uIndex const& idx, asset::Pipeline::Descriptor &descriptor)
{
	bool bChanged = false;

	ImGui::Text("%i)", idx);

	ImGui::SameLine();

	ImGui::PushItemWidth(100);
	if (gui::FieldText<64>::Inline("Identifier", descriptor.id)) bChanged = true;
	ImGui::PopItemWidth();
	
	ImGui::SameLine();
	
	ImGui::PushItemWidth(200);
	if (gui::Combo<graphics::DescriptorType::Enum>::Inline(
		"Type", graphics::DescriptorType::ALL, descriptor.type,
		graphics::DescriptorType::to_string,
		[](graphics::DescriptorType::Enum type) { ImGui::PushID((ui32)type); }
	)) bChanged = true;
	ImGui::PopItemWidth();
	
	ImGui::SameLine();
	
	ImGui::PushItemWidth(200);
	if (gui::Combo<graphics::ShaderStage::Enum>::Inline(
		"Stage", graphics::ShaderStage::ALL, descriptor.stage,
		graphics::ShaderStage::to_string,
		[](graphics::ShaderStage::Enum type) { ImGui::PushID((ui32)type); }
	)) bChanged = true;
	ImGui::PopItemWidth();
	
	return bChanged;
}

void EditorPipeline::saveAsset()
{
	this->get<asset::Pipeline>()->setDescriptorGroups(this->mDescriptorGroups);
	AssetEditor::saveAsset();
}
