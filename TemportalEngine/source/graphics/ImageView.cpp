#include "graphics/ImageView.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

ImageView::ImageView(ImageView &&other)
{
	*this = std::move(other);
}

ImageView& ImageView::operator=(ImageView &&other)
{
	this->mInternal = std::move(other.mInternal);
	return *this;
}

ImageView::~ImageView()
{
	this->mInternal.reset();
}
