#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"

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
	typedef asset::TypedAssetPath<asset::Texture> TexturePath;
	typedef asset::TypedAssetPath<asset::TextureSampler> SamplerPath;

	TextureRegistry(
		std::weak_ptr<GraphicsDevice> device,
		CommandPool *transientPool
	);
	~TextureRegistry();

	void registerImage(TexturePath const& assetPath);
	void registerSampler(SamplerPath const& assetPath);

	std::weak_ptr<ImageView> getImage(TexturePath const& assetPath);
	std::weak_ptr<ImageSampler> getSampler(SamplerPath const& assetPath);

private:
	std::weak_ptr<GraphicsDevice> mpDevice;
	CommandPool *mpTransientPool;

	struct ImageEntry
	{
		std::shared_ptr<Image> image;
		std::shared_ptr<ImageView> view;
	};
	std::unordered_map<asset::AssetPath, ImageEntry> mImages;

	struct SamplerEntry
	{
		std::shared_ptr<ImageSampler> sampler;
	};
	std::unordered_map<asset::AssetPath, SamplerEntry> mSamplers;

};

NS_END
