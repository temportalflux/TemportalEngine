#pragma once

#include "CoreInclude.hpp"

#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/Descriptor.hpp"

FORWARD_DEF(NS_RESOURCE, class PackManager);

NS_GRAPHICS
class GraphicsDevice;
class Image;
class ImageView;
class ImageSampler;
class CommandPool;

// TODO: This needs to be decoupled from assets so that resource packs can be created (reading textures and raw assets directly from user directories)
class TextureRegistry
{
public:
	typedef std::string TextureId;
	typedef asset::TypedAssetPath<asset::Texture> TexturePath;
	typedef asset::TypedAssetPath<asset::TextureSampler> SamplerPath;

	TextureRegistry(
		std::weak_ptr<GraphicsDevice> device,
		CommandPool *transientPool, DescriptorPool *descriptorPool
	);
	~TextureRegistry();

	std::function<void(resource::PackManager*)> onTexturesLoadedEvent();
	void registerImage(TextureId const& id, TexturePath const& assetPath);
	void registerSampler(SamplerPath const& assetPath);
	void setPackSampler(SamplerPath const& samplerId);
	void createDescriptor(TextureId const& textureId, SamplerPath const& samplerId);
	DescriptorLayout const* textureLayout() const;

	std::weak_ptr<ImageView> getImage(TextureId const& id);
	DescriptorSetPool::Handle const& getDescriptorHandle(TextureId const& id) const;
	std::weak_ptr<ImageSampler> getSampler(SamplerPath const& assetPath);

private:
	std::weak_ptr<GraphicsDevice> mpDevice;
	CommandPool *mpTransientPool;

	std::shared_ptr<graphics::DescriptorSetPool> mpTextureDescriptors;
	std::optional<SamplerPath> mSamplerIdForPackTextures;

	struct ImageEntry
	{
		std::shared_ptr<Image> image;
		std::shared_ptr<ImageView> view;
		graphics::DescriptorSetPool::Handle descriptorHandle;
	};
	std::unordered_map<TextureId, ImageEntry> mImages;

	struct SamplerEntry
	{
		std::shared_ptr<ImageSampler> sampler;
	};
	std::unordered_map<asset::AssetPath, SamplerEntry> mSamplers;

	void onTexturesLoaded(resource::PackManager *packManager);
	void registerImage(TextureId const& id, math::Vector2UInt const& size, std::vector<ui8> const& pixels);

};

NS_END
