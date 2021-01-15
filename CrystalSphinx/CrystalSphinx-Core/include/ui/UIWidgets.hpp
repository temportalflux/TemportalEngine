#pragma once

#include "CoreInclude.hpp"
#include "asset/TypedAssetPath.hpp"
#include "ui/Core.hpp"
#include "ui/ImageWidget.hpp"

FORWARD_DEF(NS_ASSET, class Texture);
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);

NS_UI

struct ResourceRequirements
{
	std::weak_ptr<graphics::GraphicsDevice> device;
	graphics::CommandPool* transientPool;
	graphics::DescriptorLayout *layout;
	graphics::DescriptorPool *descriptorPool;
	graphics::ImageSampler *sampler;
};

struct AssetImageResource
{
	asset::TypedAssetPath<asset::Texture> assetPath;
	std::shared_ptr<ui::ImageResource> resource;
	ResourceRequirements requirements;
	
	void create(ResourceRequirements const& requirements);
	void destroy();
};

void createResources(ResourceRequirements const& requirements);
void destroyResources();

extern std::shared_ptr<ui::ImageResource> RES_IMG_WHITE;

extern AssetImageResource ASSET_IMG_BACKGROUND;
ui::Image& createMenuBackground(ui::Image& img, ui32 borderWidthInPoints);

extern AssetImageResource ASSET_IMG_SLOT;
ui::Image& createSlotImage(ui::Image& img, ui32 borderWidthInPoints);

NS_END
