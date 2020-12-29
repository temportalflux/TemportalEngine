#include "ui/Widget.hpp"
#include "ui/WidgetRenderer.hpp"

using namespace ui;

void Widget::setZLayer(ui32 z)
{
	if (!this->mpRenderer.expired())
	{
		this->mpRenderer.lock()->changeZLayer(this->weak_from_this(), z);
	}
	this->mZLayer = z;
}

ui32 Widget::zLayer() const
{
	return this->mZLayer;
}
