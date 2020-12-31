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
	Widget();

	void setRenderer(std::weak_ptr<ui::WidgetRenderer> renderer) { this->mpRenderer = renderer; }
	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) { this->mpDevice = device; }

	Widget& setResolution(ui::Resolution const& resolution) { this->mResolution = resolution; return *this; }
	ui::Resolution const& resolution() const { return this->mResolution; }

	Widget& setAnchorParent(std::weak_ptr<ui::Widget> parent);
	Widget& setAnchor(math::Vector2 const& anchor);
	Widget& setPivot(math::Vector2 const& pivot);
	Widget& setPosition(math::Vector2Int const& points);
	Widget& setSize(math::Vector2UInt const& points);
	Widget& setZLayer(ui32 z);
	ui32 zLayer() const;

	math::Vector2 getTopLeftPositionOnScreen() const;
	math::Vector2 getSizeOnScreen() const;

	virtual Widget& create() { return *this; }
	virtual Widget& commit(graphics::CommandPool* transientPool) { return *this; }

	virtual void bind(graphics::Command *command, std::shared_ptr<graphics::Pipeline> pipeline) {};
	virtual void record(graphics::Command *command) {};

private:
	std::weak_ptr<ui::WidgetRenderer> mpRenderer;
	std::weak_ptr<graphics::GraphicsDevice> mpDevice;
	ui::Resolution mResolution;

	std::weak_ptr<ui::Widget> mpAnchorParent;

	/**
	 * The position of the widget's anchor as a fraction of the screen size.
	 * 0 means left/top, 1 means right/bottom
	 */
	math::Vector2 mAnchor;
	/**
	 * The position of the widget's render position relative to its size.
	 * <0, 0> means the "position in points" is the top left of the widget.
	 * <0.5, 0.5> means the "position in points" is the center of the widget (based on "size in points").
	 * <1, 1> means the "position in points" is the bottom right of the widget.
	 */
	math::Vector2 mPivot;
	/**
	 * The position of the widget from the anchor.
	 * The true top-left of the widget is based on this and the `pivot`.
	 */
	math::Vector2Int mPositionInPoints;
	math::Vector2Int mSizeInPoints;
	ui32 mZLayer;

};

NS_END
