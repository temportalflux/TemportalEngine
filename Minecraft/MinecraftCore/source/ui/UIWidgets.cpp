#include "ui/UIWidgets.hpp"

#include "asset/Texture.hpp"

asset::TypedAssetPath<asset::Texture> ui::ASSET_IMG_BACKGROUND = asset::TypedAssetPath<asset::Texture>::Create(
	"assets/textures/ui/background.te-asset"
);

ui::Image& ui::createMenuBackground(ui::Image& img, ui32 borderWidthInPoints)
{
	auto asset = ui::ASSET_IMG_BACKGROUND.load(asset::EAssetSerialization::Binary);
	ui::Image::Slice border = { borderWidthInPoints, 5 };
	img
		.setTextureSize(asset->getSourceSize())
		.setTexturePixels(asset->getSourceBinary())
		.setTexturePadding({ 0, 0 })
		.setTextureSubsize(asset->getSourceSize())
		.setTextureSlicing({ {
			border, // left
			border, // right
			border, // top
			border, // bottom
		} });
	return img;
}
