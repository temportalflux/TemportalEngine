#pragma once

#include "asset/TypedAssetPath.hpp"
#include "asset/PipelineAsset.hpp"
#include "graphics/Descriptor.hpp"
#include "graphics/ImageSampler.hpp"
#include "ui/Core.hpp"
#include "ui/Resolution.hpp"

FORWARD_DEF(NS_GRAPHICS, class Command);
FORWARD_DEF(NS_GRAPHICS, class CommandPool);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);

NS_UI
class Image;

class ImageWidgetRenderer
{

public:
	ImageWidgetRenderer();
	~ImageWidgetRenderer();

	ImageWidgetRenderer& setImagePipeline(asset::TypedAssetPath<asset::Pipeline> const& path);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);
	void initializeData(graphics::CommandPool *pool, graphics::DescriptorPool *descriptorPool);

	void add(std::weak_ptr<ui::Image> widget);
	void removeExpired();

	std::shared_ptr<graphics::Pipeline>& imagePipeline() { return this->mImagePipeline; }
	graphics::DescriptorLayout& imageDescriptorLayout() { return this->mImageDescriptorLayout; }
	graphics::ImageSampler& imageSampler() { return this->mImageSampler; }

	void createPipeline(math::Vector2UInt const& resolution);
	void record(graphics::Command *command);

private:
	std::weak_ptr<graphics::GraphicsDevice> mpDevice;
	graphics::CommandPool* mpTransientPool;
	graphics::DescriptorPool* mpDescriptorPool;
	ui::Resolution mResolution;

	std::shared_ptr<graphics::Pipeline> mImagePipeline;
	graphics::DescriptorLayout mImageDescriptorLayout;
	graphics::ImageSampler mImageSampler;

	std::vector<std::weak_ptr<ui::Image>> mImageWidgets;

	void initializeData_image(std::shared_ptr<ui::Image> img);
	void commit_image(std::shared_ptr<ui::Image> img);

};

NS_END
