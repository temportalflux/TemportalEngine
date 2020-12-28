#include "ui/WidgetRenderer.hpp"

#include "graphics/assetHelpers.hpp"
#include "graphics/Command.hpp"
#include "graphics/Pipeline.hpp"
#include "ui/ImageWidget.hpp"

ui::ImageWidgetRenderer::ImageWidgetRenderer()
	: mResolution({})
{

}

ui::ImageWidgetRenderer::~ImageWidgetRenderer()
{

}

void ui::ImageWidgetRenderer::add(std::weak_ptr<ui::Image> widget)
{
	this->mImageWidgets.push_back(widget);
	widget.lock()->setDevice(this->mpDevice);
	if (this->mpTransientPool != nullptr) this->initializeData_image(widget.lock());
	if (this->mResolution.dotsPerInch > 0) this->commit_image(widget.lock());
}

void ui::ImageWidgetRenderer::removeExpired()
{
	this->mImageWidgets.erase(std::remove_if(
		this->mImageWidgets.begin(), this->mImageWidgets.end(),
		[](auto const& w) { return w.expired(); }
	));
}

ui::ImageWidgetRenderer& ui::ImageWidgetRenderer::setImagePipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->imagePipeline())
	{
		this->imagePipeline() = std::make_shared<graphics::Pipeline>();
	}

	graphics::populatePipeline(path, this->imagePipeline().get(), &this->imageDescriptorLayout());

	{
		ui8 slot = 0;
		this->imagePipeline()->setBindings({
			graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
			.setStructType<ui::Image::Vertex>()
			.addAttribute({ 0, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(ui::Image::Vertex, position) })
			.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(ui::Image::Vertex, textureCoordinate) })
		});
	}

	return *this;
}

void ui::ImageWidgetRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpDevice = device;
	this->imageSampler().setDevice(device);
	this->imageSampler().create();
	this->imagePipeline()->setDevice(device);
	this->imageDescriptorLayout().setDevice(device).create();
	for (auto imgPtr : this->mImageWidgets)
	{
		imgPtr.lock()->setDevice(device);
	}
}

void ui::ImageWidgetRenderer::initializeData(graphics::CommandPool *pool, graphics::DescriptorPool *descriptorPool)
{
	this->mpTransientPool = pool;
	this->mpDescriptorPool = descriptorPool;

	for (auto imgPtr : this->mImageWidgets)
	{
		this->initializeData_image(imgPtr.lock());
	}
}

void ui::ImageWidgetRenderer::initializeData_image(std::shared_ptr<ui::Image> img)
{
	img
		->create(this->mpTransientPool)
		.createDescriptor(this->imageDescriptorLayout(), this->mpDescriptorPool)
		.attachWithSampler(&this->imageSampler());
}

void ui::ImageWidgetRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mResolution = { resolution, 96 };

	this->imagePipeline()
		->setDescriptorLayout(this->imageDescriptorLayout(), 1)
		.setResolution(resolution)
		.create();

	for (auto imgPtr : this->mImageWidgets)
	{
		this->commit_image(imgPtr.lock());
	}
}

void ui::ImageWidgetRenderer::commit_image(std::shared_ptr<ui::Image> img)
{
	img->setResolution(this->mResolution).commit(this->mpTransientPool);
}

void ui::ImageWidgetRenderer::record(graphics::Command *command)
{
	OPTICK_EVENT()
	bool bHasExpired = false;
	command->bindPipeline(this->imagePipeline());
	for (auto weak_img : this->mImageWidgets)
	{
		if (weak_img.expired())
		{
			bHasExpired = true;
			continue;
		}
		auto pImg = weak_img.lock();
		pImg->bind(command, this->imagePipeline());
		pImg->record(command);
	}
	if (bHasExpired)
	{
		this->removeExpired();
	}
}
