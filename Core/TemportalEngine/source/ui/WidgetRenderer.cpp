#include "ui/WidgetRenderer.hpp"

#include "game/GameClient.hpp"
#include "game/GameInstance.hpp"
#include "graphics/assetHelpers.hpp"
#include "graphics/Command.hpp"
#include "graphics/Pipeline.hpp"
#include "render/MinecraftRenderer.hpp"
#include "ui/ImageWidget.hpp"
#include "ui/TextWidget.hpp"

ui::WidgetRenderer::WidgetRenderer()
	: mResolution({})
	, mbAnyWidgetHasChanges(false)
{

}

ui::WidgetRenderer::~WidgetRenderer()
{

}

ui::WidgetRenderer::LayerFindResult ui::WidgetRenderer::findLayer(ui32 z)
{
	return std::equal_range(this->mLayers.begin(), this->mLayers.end(), Layer { z });
}

bool ui::WidgetRenderer::Layer::operator<(Layer const& other) const { return this->z < other.z; }

ui::WidgetRenderer::Layer& ui::WidgetRenderer::getOrMakeLayer(ui32 z)
{
	auto layerResult = this->findLayer(z);
	std::vector<Layer>::iterator iter = layerResult.first;
	if (std::distance(layerResult.first, layerResult.second) == 0)
	{
		iter = this->mLayers.insert(layerResult.first, Layer { z, {} });
	}
	return *iter;
}

void ui::WidgetRenderer::add(std::weak_ptr<ui::Widget> widget)
{
	auto pWidget = widget.lock();
	auto& layer = this->getOrMakeLayer(pWidget->zLayer());
	layer.widgets.push_back(widget);

	pWidget->setRenderer(weak_from_this());
	pWidget->setDevice(this->mpDevice);
	if (this->mpTransientPool != nullptr) this->initializeWidgetData(pWidget);
	if (this->mResolution.dotsPerInch > 0) this->commitWidget(pWidget);
}

void ui::WidgetRenderer::changeZLayer(std::weak_ptr<ui::Widget> widget, ui32 newZ)
{
	auto oldZ = widget.lock()->zLayer();
	auto layerResult = this->findLayer(oldZ);
	assert(std::distance(layerResult.first, layerResult.second) != 0);

	auto& widgets = layerResult.first->widgets;
	widgets.erase(std::remove_if(
		widgets.begin(), widgets.end(),
		[widget](auto const& w) { return w.lock() == widget.lock(); }
	));
	
	auto& layer = this->getOrMakeLayer(newZ);
	layer.widgets.push_back(widget);
}

ui::WidgetRenderer& ui::WidgetRenderer::setImagePipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->imagePipeline())
	{
		this->imagePipeline() = std::make_shared<graphics::Pipeline>();
	}

	this->imagePipeline()->setDepthEnabled(false, false);
	graphics::populatePipeline(path, this->imagePipeline().get(), &this->imageDescriptorLayout());
	
	this->imagePipeline()->setBindings({
		graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
		.setStructType<ui::Image::Vertex>()
		.addAttribute({ 0, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(ui::Image::Vertex, position) })
		.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(ui::Image::Vertex, textureCoordinate) })
		.addAttribute({ 2, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(ui::Image::Vertex, color) })
	});

	return *this;
}

ui::WidgetRenderer& ui::WidgetRenderer::setTextPipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->textPipeline())
	{
		this->textPipeline() = std::make_shared<graphics::Pipeline>();
	}

	graphics::populatePipeline(path, this->textPipeline().get(), &this->textDescriptorLayout());
	this->textPipeline()->setDepthEnabled(false, false);
	this->textPipeline()->setBindings({ ui::Text::binding() });

	return *this;
}

void ui::WidgetRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpDevice = device;
	this->imageSampler().setDevice(device);
	this->imageSampler().create();
	this->imagePipeline()->setDevice(device);
	this->imageDescriptorLayout().setDevice(device).create();

	this->textSampler().setDevice(device);
	this->textSampler().create();
	this->textPipeline()->setDevice(device);
	this->textDescriptorLayout().setDevice(device).create();

	for (auto const& layer : this->mLayers)
	{
		for (auto const& widget : layer.widgets)
		{
			widget.lock()->setDevice(device);
		}
	}
}

void ui::WidgetRenderer::initializeData(graphics::CommandPool *pool, graphics::DescriptorPool *descriptorPool)
{
	this->mpTransientPool = pool;
	this->mpDescriptorPool = descriptorPool;

	for (auto const& layer : this->mLayers)
	{
		for (auto const& widget : layer.widgets)
		{
			this->initializeWidgetData(widget.lock());
		}
	}
}

void ui::WidgetRenderer::initializeWidgetData(std::shared_ptr<ui::Widget> widget)
{
	widget->create();
}

void ui::WidgetRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mResolution = { resolution, game::Game::Get()->client()->renderer()->dpi() };

	this->imagePipeline()
		->setDescriptorLayout(this->imageDescriptorLayout(), 1)
		.setResolution(resolution)
		.create();
	this->textPipeline()
		->setDescriptorLayout(this->textDescriptorLayout(), 1)
		.setResolution(resolution)
		.create();

	this->commitWidgets();
}

bool ui::WidgetRenderer::hasChanges() const { return this->mbAnyWidgetHasChanges; }
void ui::WidgetRenderer::setAnyWidgetIsDirty()
{
	this->mMutex.lock();
	this->mbAnyWidgetHasChanges = true;
	this->mMutex.unlock();
}

void ui::WidgetRenderer::commitWidgets()
{
	this->mMutex.lock();

	for (auto const& layer : this->mLayers)
	{
		for (auto const& widget : layer.widgets)
		{
			this->commitWidget(widget.lock());
		}
	}

	this->mbAnyWidgetHasChanges = false;
	this->mMutex.unlock();
}

void ui::WidgetRenderer::commitWidget(std::shared_ptr<ui::Widget> widget)
{
	if (widget->hasChanges())
	{
		widget->setResolution(this->mResolution).commit();
	}
}

void ui::WidgetRenderer::record(graphics::Command *command)
{
	OPTICK_EVENT()
	for (auto it = this->mLayers.begin(); it != this->mLayers.end(); ++it)
	{
		bool bHasAnyExpired = false;
		for (auto weak_img : it->widgets)
		{
			if (weak_img.expired())
			{
				bHasAnyExpired = true;
				continue;
			}
			auto pImg = weak_img.lock();
			if (pImg->isVisible())
			{
				pImg->record(command);
			}
		}
		if (bHasAnyExpired)
		{
			it->widgets.erase(std::remove_if(
				it->widgets.begin(), it->widgets.end(),
				[](auto const& w) { return w.expired(); }
			));
		}
	}
}
