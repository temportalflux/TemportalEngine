#include "ui/UIWidgets.hpp"

#include "asset/Texture.hpp"

ui::AssetImageResource ui::ASSET_IMG_BACKGROUND = {
	asset::TypedAssetPath<asset::Texture>::Create("assets/textures/ui/background.te-asset")
};
ui::AssetImageResource ui::ASSET_IMG_SLOT = {
	asset::TypedAssetPath<asset::Texture>::Create("assets/textures/ui/slot.te-asset")
};

void ui::AssetImageResource::create(ResourceRequirements const& requirements)
{
	auto asset = this->assetPath.load(asset::EAssetSerialization::Binary);
	(this->resource = std::make_shared<ui::ImageResource>())
		->setDevice(requirements.device)
		.setTextureSize(asset->getSourceSize())
		.setTexturePixels(asset->getSourceBinary())
		.create(requirements.transientPool)
		.createDescriptor(requirements.layout, requirements.descriptorPool)
		.attachWithSampler(requirements.sampler);
}

void ui::AssetImageResource::destroy()
{
	this->resource.reset();
}

void ui::createResources(ResourceRequirements const& requirements)
{
	ui::ASSET_IMG_BACKGROUND.create(requirements);
	ui::ASSET_IMG_SLOT.create(requirements);
}

void ui::destroyResources()
{
	ui::ASSET_IMG_BACKGROUND.destroy();
	ui::ASSET_IMG_SLOT.destroy();
}

ui::Image& ui::createMenuBackground(ui::Image& img, ui32 borderWidthInPoints)
{
	ui::Image::Slice border = { borderWidthInPoints, 5 };
	img
		.setResource(ui::ASSET_IMG_BACKGROUND.resource)
		.setTextureSubsize(ui::ASSET_IMG_BACKGROUND.resource->size())
		.setTexturePadding({ 0, 0 })
		.setTextureSlicing({ {
			border, // left
			border, // right
			border, // top
			border, // bottom
		} });
	return img;
}

ui::Image& ui::createSlotImage(ui::Image& img, ui32 borderWidthInPoints)
{
	ui::Image::Slice border = { borderWidthInPoints, 1 };
	img
		.setResource(ui::ASSET_IMG_SLOT.resource)
		.setTextureSubsize(ui::ASSET_IMG_SLOT.resource->size())
		.setTexturePadding({ 0, 0 })
		.setTextureSlicing({ {
			border, // left
			border, // right
			border, // top
			border, // bottom
		} });
	return img;
}
