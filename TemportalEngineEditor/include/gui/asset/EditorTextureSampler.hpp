#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "gui/widget/Combo.hpp"
#include "graphics/types.hpp"
#include "gui/widget/FieldNumber.hpp"

NS_GUI

// Editor for `asset::TextureSampler`
class EditorTextureSampler : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;

protected:
	void renderContent() override;
	void saveAsset() override;

private:
	union FilterModeCombos
	{
		struct
		{
			gui::Combo<graphics::FilterMode::Enum> magnified, minified;
		};
		gui::Combo<graphics::FilterMode::Enum> combos[2];
		FilterModeCombos() {}
		~FilterModeCombos() {}
	} mFilterModes;
	union AddressModeCombos
	{
		struct
		{
			gui::Combo<graphics::SamplerAddressMode::Enum> u, v, w;
		};
		gui::Combo<graphics::SamplerAddressMode::Enum> combos[3];
		AddressModeCombos() {}
		~AddressModeCombos() {}
	} mAddressModes;
	std::optional<f32> mAnisotropyValue;
	gui::Combo<graphics::BorderColor::Enum> mBorderColor;
	bool mUseNormalizedCoords;
	std::optional<graphics::CompareOp::Enum> mCompareOpValue;
	gui::Combo<graphics::CompareOp::Enum> mCompareOpCombo;
	gui::Combo<graphics::SamplerLODMode::Enum> mMipLODMode;
	gui::FieldNumber<f32, 1> mLodBias;
	gui::FieldNumber<f32, 2> mLodRange;

};

NS_END
