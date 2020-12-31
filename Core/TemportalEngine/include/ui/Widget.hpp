#pragma once

#include "ui/Core.hpp"
#include "ui/Resolution.hpp"

FORWARD_DEF(NS_GRAPHICS, class Command);
FORWARD_DEF(NS_GRAPHICS, class CommandPool);
FORWARD_DEF(NS_GRAPHICS, class DescriptorLayout);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);
FORWARD_DEF(NS_GRAPHICS, class ImageSampler);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);

NS_UI
class WidgetRenderer;

class Widget : public std::enable_shared_from_this<Widget>
{
public:
	Widget() : mZLayer(0) {}

	void setRenderer(std::weak_ptr<ui::WidgetRenderer> renderer) { this->mpRenderer = renderer; }
	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) { this->mpDevice = device; }
	Widget& setResolution(ui::Resolution const& resolution) { this->mResolution = resolution; return *this; }

	void setRenderPosition(math::Vector2Int const& points) { this->mRenderPositionInPoints = points; }
	void setZLayer(ui32 z);
	ui32 zLayer() const;
	void setRenderSize(math::Vector2UInt const& points) { this->mRenderSizeInPoints = points; }

	ui::Resolution const& resolution() const { return this->mResolution; }
	math::Vector2 getTopLeftPositionOnScreen() const
	{
		return this->mResolution.pointsToScreenSpace(this->mRenderPositionInPoints);
	}
	math::Vector2 getSizeOnScreen() const
	{
		return this->mResolution.pointsToScreenSpace({
			i32(this->mRenderSizeInPoints.x()),
			i32(this->mRenderSizeInPoints.y())
		});
	}

	virtual Widget& create(graphics::CommandPool* transientPool) { return *this; }
	virtual Widget& createDescriptor(graphics::DescriptorLayout *layout, graphics::DescriptorPool *descriptorPool) { return *this; }
	virtual Widget& attachWithSampler(graphics::ImageSampler *sampler) { return *this; }
	virtual Widget& commit(graphics::CommandPool* transientPool) { return *this; }

	virtual void bind(graphics::Command *command, std::shared_ptr<graphics::Pipeline> pipeline) {};
	virtual void record(graphics::Command *command) {};

private:
	std::weak_ptr<ui::WidgetRenderer> mpRenderer;
	std::weak_ptr<graphics::GraphicsDevice> mpDevice;
	ui::Resolution mResolution;

	math::Vector2Int mRenderPositionInPoints;
	ui32 mZLayer;
	math::Vector2UInt mRenderSizeInPoints;

};

NS_END
