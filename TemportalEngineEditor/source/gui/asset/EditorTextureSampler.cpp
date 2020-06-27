#include "gui/asset/EditorTextureSampler.hpp"

#include "asset/TextureSampler.hpp"
#include "memory/MemoryChunk.hpp"
#include "gui/widget/Optional.hpp"

#include <imgui.h>

using namespace gui;

#define Bit_Dirty 1 << 0

std::shared_ptr<AssetEditor> EditorTextureSampler::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorTextureSampler>();
}

void EditorTextureSampler::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::TextureSampler>();

	// Initialize filter modes
	{
		auto pushId = [](graphics::FilterMode::Enum v) { ImGui::PushID((ui32)v); };
		for (auto& combo : this->mFilterModes.combos)
		{
			combo
				.setOptions(graphics::FilterMode::ALL)
				.setCallbacks(&graphics::FilterMode::to_string, pushId);
		}
		this->mFilterModes.magnified.value(asset->getFilterModeMagnified());
		this->mFilterModes.minified.value(asset->getFilterModeMinified());
	}
	// Initialize address modes
	{

		auto pushId = [](graphics::SamplerAddressMode::Enum v) { ImGui::PushID((ui32)v); };
		for (auto& combo : this->mAddressModes.combos)
		{
			combo
				.setOptions(graphics::SamplerAddressMode::ALL)
				.setCallbacks(&graphics::SamplerAddressMode::to_string, pushId);
		}
		auto addressModeValues = asset->getAddressModes();
		this->mAddressModes.u.value(addressModeValues[0]);
		this->mAddressModes.v.value(addressModeValues[1]);
		this->mAddressModes.w.value(addressModeValues[2]);
	}
	// Anisotropy
	this->mAnisotropyValue = asset->getAnisotropy();
	// Border Color
	{
		this->mBorderColor
			.setOptions(graphics::BorderColor::ALL)
			.setCallbacks(
				&graphics::BorderColor::to_string,
				[](graphics::BorderColor::Enum v) { ImGui::PushID((ui32)v); }
			)
			.value(asset->getBorderColor());
	}
	// Normalized Coords
	this->mUseNormalizedCoords = asset->areCoordinatesNormalized();
	// Compare Op (Value)
	{
		this->mCompareOpValue = asset->getCompareOperation();
		this->mCompareOpCombo
			.setOptions(graphics::CompareOp::ALL)
			.setCallbacks(
				&graphics::CompareOp::to_string,
				[](graphics::CompareOp::Enum v) { ImGui::PushID((ui32)v); }
			)
			.value(
				this->mCompareOpValue
				? *this->mCompareOpValue
				: graphics::CompareOp::Enum::Always
			);
	}
	// LOD Mode
	{
		this->mMipLODMode
			.setOptions(graphics::SamplerLODMode::ALL)
			.setCallbacks(
				&graphics::SamplerLODMode::to_string,
				[](graphics::SamplerLODMode::Enum v) { ImGui::PushID((ui32)v); }
			)
			.value(asset->getLodMode());
		this->mLodBias.value({ asset->getLodBias() });
		this->mLodRange.value({ asset->getLodRange().x(), asset->getLodRange().y() });
	}

}

void EditorTextureSampler::renderContent()
{
	AssetEditor::renderContent();

	// Filter Modes
	{
		if (this->mFilterModes.magnified.render("Filter Mode (Magnified)###filterMag"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
		if (this->mFilterModes.minified.render("Filter Mode (Minified)###filterMin"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
	}
	// Address Modes
	if (ImGui::TreeNode("Address Modes"))
	{
		if (this->mAddressModes.u.render("U"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
		if (this->mAddressModes.v.render("V"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
		if (this->mAddressModes.w.render("W"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
		ImGui::TreePop();
	}
	// Anisotropy
	if (gui::Optional<f32>::Inline(this->mAnisotropyValue, "Anisotropy", true, [](auto& amount) {
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		auto bChanged = gui::FieldNumber<f32, 1>::InlineSingle("Amount", amount);
		ImGui::PopItemWidth();
		return bChanged;
	}))
	{
		this->markAssetDirty(Bit_Dirty);
	}
	// Border Color
	if (this->mBorderColor.render("Out of Bounds Color"))
	{
		this->markAssetDirty(Bit_Dirty);
	}
	// Normalized Coords
	if (ImGui::Checkbox("Normalize Coordinates", &this->mUseNormalizedCoords))
	{
		this->markAssetDirty(0);
	}
	// Compare Operation
	if (gui::Optional<graphics::CompareOp::Enum>::Inline(
		this->mCompareOpValue, "Compare Operation", true, [&](auto& op) {
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		if (this->mCompareOpCombo.value(op).render("##operation"))
		{
			op = this->mCompareOpCombo.value();
			ImGui::PopItemWidth();
			return true;
		}
		else
		{
			ImGui::PopItemWidth();
			return false;
		}
	}))
	{
		this->markAssetDirty(Bit_Dirty);
	}
	// LOD
	if (ImGui::TreeNode("Level of Detail"))
	{
		// Mode
		if (this->mMipLODMode.render("Mode"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
		// Bias
		if (this->mLodBias.render("Bias"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
		// Range
		if (this->mLodRange.render("Range"))
		{
			this->markAssetDirty(Bit_Dirty);
		}
		// End
		ImGui::TreePop();
	}

}

void EditorTextureSampler::saveAsset()
{
	auto asset = this->get<asset::TextureSampler>();
	asset->setFilterModeMagnified(this->mFilterModes.magnified.value());
	asset->setFilterModeMinified(this->mFilterModes.minified.value());
	asset->setAddressModes({
		this->mAddressModes.u.value(),
		this->mAddressModes.v.value(),
		this->mAddressModes.w.value()
	});
	asset->setAnisotropy(this->mAnisotropyValue);
	asset->setBorderColor(this->mBorderColor.value());
	asset->setCoordinatesNormalized(this->mUseNormalizedCoords);
	asset->setCompareOperation(this->mCompareOpValue);
	asset->setLodMode(this->mMipLODMode.value());
	asset->setLodBias(this->mLodBias[0]);
	asset->setLodRange({ this->mLodRange[0], this->mLodRange[1] });
	AssetEditor::saveAsset();
}
