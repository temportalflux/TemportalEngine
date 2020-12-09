#include "render/TextureRegistry.hpp"

#include "graphics/assetHelpers.hpp"
#include "graphics/CommandPool.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/ImageSampler.hpp"
#include "resource/ResourceManager.hpp"

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

void TextureRegistry::registerImage(TextureId const& id, TexturePath const& assetPath)
{
	auto asset = assetPath.load(asset::EAssetSerialization::Binary);
	this->registerImage(id, asset->getSourceSize(), asset->getSourceBinary());
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

std::weak_ptr<ImageView> TextureRegistry::getImage(TextureId const& id)
{
	auto iter = this->mImages.find(id);
	return iter != this->mImages.end() ? iter->second.view : std::weak_ptr<ImageView>();
}

std::weak_ptr<ImageSampler> TextureRegistry::getSampler(SamplerPath const& assetPath)
{
	auto iter = this->mSamplers.find(assetPath.path());
	return iter != this->mSamplers.end() ? iter->second.sampler : std::weak_ptr<ImageSampler>();
}

std::function<void(resource::PackManager*)> TextureRegistry::onTexturesLoadedEvent()
{
	return std::bind(&TextureRegistry::onTexturesLoaded, this, std::placeholders::_1);
}

void TextureRegistry::onTexturesLoaded(resource::PackManager *packManager)
{
	auto entries = packManager->getTexturesOfType("model");
	for (auto const& entry : entries)
	{
		auto const& data = packManager->getTextureData(entry);
		this->registerImage(entry.textureId(), data.size, data.pixels);
	}
}

void TextureRegistry::registerImage(TextureId const& id, math::Vector2UInt const& size, std::vector<ui8> const& pixels)
{
	ImageEntry entry = {
		std::make_shared<Image>(),
		std::make_shared<ImageView>()
	};
	entry.image->setDevice(this->mpDevice);
	entry.view->setDevice(this->mpDevice);

	entry.image
		->setFormat(vk::Format::eR8G8B8A8Srgb)
		.setSize(math::Vector3UInt(size).z(1))
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	entry.image->create();

	entry.image->transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, this->mpTransientPool);
	entry.image->writeImage((void*)pixels.data(), pixels.size() * sizeof(ui8), this->mpTransientPool);
	entry.image->transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, this->mpTransientPool);

	entry.view->setImage(entry.image.get(), vk::ImageAspectFlagBits::eColor);
	entry.view->create();

	this->mImages.insert(std::make_pair(id, std::move(entry)));
}
