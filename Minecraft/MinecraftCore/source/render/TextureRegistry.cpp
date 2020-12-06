#include "render/TextureRegistry.hpp"

#include "graphics/assetHelpers.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/ImageSampler.hpp"

using namespace graphics;

TextureRegistry::TextureRegistry(
	std::weak_ptr<GraphicsDevice> device,
	CommandPool *transientPool
)
	: mpDevice(device)
	, mpTransientPool(transientPool)
{
}

TextureRegistry::~TextureRegistry()
{
	this->mImages.clear();
	this->mSamplers.clear();
}

void TextureRegistry::registerImage(TexturePath const& assetPath)
{
	auto entry = ImageEntry{
		std::make_shared<Image>(),
		std::make_shared<ImageView>()
	};
	entry.image->setDevice(this->mpDevice);
	entry.view->setDevice(this->mpDevice);

	auto pixelData = graphics::populateImage(entry.image.get(), assetPath);
	entry.image->create();

	entry.image->transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, this->mpTransientPool);
	entry.image->writeImage((void*)pixelData.data(), pixelData.size() * sizeof(ui8), this->mpTransientPool);
	entry.image->transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, this->mpTransientPool);

	entry.view->setImage(entry.image.get(), vk::ImageAspectFlagBits::eColor);
	entry.view->create();

	this->mImages.insert(std::make_pair(assetPath.path(), std::move(entry)));
}

void TextureRegistry::registerSampler(SamplerPath const& assetPath)
{
	auto entry = SamplerEntry{
		std::make_shared<ImageSampler>()
	};
	entry.sampler->setDevice(this->mpDevice);
	graphics::populateSampler(entry.sampler.get(), assetPath);
	entry.sampler->create();
	this->mSamplers.insert(std::make_pair(assetPath.path(), std::move(entry)));
}

std::weak_ptr<ImageView> TextureRegistry::getImage(TexturePath const& assetPath)
{
	auto iter = this->mImages.find(assetPath.path());
	return iter != this->mImages.end() ? iter->second.view : std::weak_ptr<ImageView>();
}

std::weak_ptr<ImageSampler> TextureRegistry::getSampler(SamplerPath const& assetPath)
{
	auto iter = this->mSamplers.find(assetPath.path());
	return iter != this->mSamplers.end() ? iter->second.sampler : std::weak_ptr<ImageSampler>();
}
