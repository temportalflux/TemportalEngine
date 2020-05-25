#include "graphics/ImageView.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

ImageView::~ImageView()
{
	this->mInternal.reset();
}

void ImageView::setInFlightFence(vk::Fence &fence)
{
	this->mFence_ImageInFlight = fence;
}

bool ImageView::isInFlight() const
{
	return (bool)this->mFence_ImageInFlight;
}

void ImageView::waitUntilNotInFlight(LogicalDevice const *pDevice)
{
	pDevice->mDevice->waitForFences(this->mFence_ImageInFlight, true, UINT64_MAX);
}
