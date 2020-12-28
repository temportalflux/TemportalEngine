#pragma once

#include "ui/Core.hpp"
#include "ui/Resolution.hpp"

FORWARD_DEF(NS_GRAPHICS, class Command);
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice);
FORWARD_DEF(NS_GRAPHICS, class Pipeline);

NS_UI

enum class EAlignment
{
	eLeft,
	eCenter,
	eRight,
	eFill,
};

class Widget
{
public:

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) { this->mpDevice = device; }
	void setResolution(ui::Resolution const& resolution) { this->mResolution = resolution; }

	void setRenderPosition(math::Vector2Int const& points) { this->mRenderPositionInPoints = points; }
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

	virtual void bind(graphics::Command *command, std::shared_ptr<graphics::Pipeline> pipeline) {};
	virtual void record(graphics::Command *command) {};

private:
	std::weak_ptr<graphics::GraphicsDevice> mpDevice;
	ui::Resolution mResolution;

	math::Vector2Int mRenderPositionInPoints;
	math::Vector2UInt mRenderSizeInPoints;

};

NS_END
