#include "ui/Widget.hpp"
#include "ui/WidgetRenderer.hpp"

using namespace ui;

Widget::Widget()
	: mAnchor({ 0, 0 })
	, mPivot({ 0, 0 })
	, mZLayer(0)
{
}

Widget& Widget::setAnchor(math::Vector2 const& anchor)
{
	this->mAnchor = anchor;
	return *this;
}

Widget& Widget::setPivot(math::Vector2 const& pivot)
{
	this->mPivot = pivot;
	return *this;
}

Widget& Widget::setPosition(math::Vector2Int const& points)
{
	this->mPositionInPoints = points;
	return *this;
}

Widget& Widget::setSize(math::Vector2UInt const& points)
{
	this->mSizeInPoints = { i32(points.x()), i32(points.y()) };
	return *this;
}

Widget& Widget::setZLayer(ui32 z)
{
	if (!this->mpRenderer.expired())
	{
		this->mpRenderer.lock()->changeZLayer(this->weak_from_this(), z);
	}
	this->mZLayer = z;
	return *this;
}

ui32 Widget::zLayer() const
{
	return this->mZLayer;
}

math::Vector2 Widget::getTopLeftPositionOnScreen() const
{
	// This function returns a vector whose components are [-1,1] indicating the fractional position on the screen (based on its resolution).
	
	// anchor is already in [0,1] space, so it needs to be converted
	auto screenPos = (this->mAnchor * 2.0f) - 1.0f;

	// the position is the offset from the anchor.
	// it is in point space, so that needs to be converted to screen space
	screenPos += this->mResolution.pointsToScreenSpace(this->mPositionInPoints);

	// Then the pivot of the widget needs to be taken into account
	// Pivot is based on the size of the widget, so convert the size into screen space and determine how much offset there is.
	screenPos -= this->getSizeOnScreen() * this->mPivot;

	return screenPos;
}

math::Vector2 Widget::getSizeOnScreen() const
{
	return this->mResolution.pointsToScreenSpace(this->mSizeInPoints);
}
