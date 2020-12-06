#include "graphics/assetHelpers.hpp"

#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageSampler.hpp"

std::vector<ui8> graphics::populateImage(graphics::Image* image, asset::TypedAssetPath<asset::Texture> const& path)
{
	auto asset = path.load(asset::EAssetSerialization::Binary);
	image
		->setFormat(vk::Format::eR8G8B8A8Srgb)
		.setSize(math::Vector3UInt(asset->getSourceSize()).z(1))
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	return asset->getSourceBinary();
}

void graphics::populateSampler(graphics::ImageSampler* sampler, asset::TypedAssetPath<asset::TextureSampler> const& path)
{
	auto samplerAsset = path.load(asset::EAssetSerialization::Binary);
	sampler
		->setFilter(
			samplerAsset->getFilterModeMagnified(),
			samplerAsset->getFilterModeMinified()
		)
		.setAddressMode(samplerAsset->getAddressModes())
		.setAnistropy(samplerAsset->getAnisotropy())
		.setBorderColor(samplerAsset->getBorderColor())
		.setNormalizeCoordinates(samplerAsset->areCoordinatesNormalized())
		.setCompare(samplerAsset->getCompareOperation())
		.setMipLOD(
			samplerAsset->getLodMode(),
			samplerAsset->getLodBias(), samplerAsset->getLodRange()
		);
}
