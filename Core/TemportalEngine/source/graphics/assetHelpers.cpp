#include "graphics/assetHelpers.hpp"

#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/Descriptor.hpp"

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

void graphics::populatePipeline(
	asset::TypedAssetPath<asset::Pipeline> const& path,
	graphics::Pipeline* pipeline, graphics::DescriptorLayout* layout
)
{
	auto asset = path.load(asset::EAssetSerialization::Binary);

	pipeline->addViewArea(asset->getViewport(), asset->getScissor());
	pipeline->setBlendMode(asset->getBlendMode());
	pipeline->setFrontFace(asset->getFrontFace());
	pipeline->setTopology(asset->getTopology());
	pipeline->setLineWidth(asset->getLineWidth());

	// Perform a synchronous load on each shader to create the shader modules
	pipeline->addShader(asset->getVertexShader().load(asset::EAssetSerialization::Binary)->makeModule());
	pipeline->addShader(asset->getFragmentShader().load(asset::EAssetSerialization::Binary)->makeModule());

	if (layout != nullptr)
	{
		uSize descCount = 0;
		for (auto const& assetDescGroup : asset->getDescriptorGroups())
		{
			descCount += assetDescGroup.descriptors.size();
		}
		layout->setBindingCount(descCount);
		descCount = 0;
		for (auto const& assetDescGroup : asset->getDescriptorGroups())
		{
			auto const& descriptors = assetDescGroup.descriptors;
			for (uIndex i = 0; i < descriptors.size(); ++i)
			{
				layout->setBinding(
					descCount + i, descriptors[i].id,
					descriptors[i].type, descriptors[i].stage, 1
				);
				descCount++;
			}
		}
	}
}
