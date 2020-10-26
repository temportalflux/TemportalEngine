#include "graphics/SwapChainSupport.hpp"

#include "types/math.h"

using namespace graphics;

ui32 SwapChainSupport::getImageViewCount() const
{
	// Adding at least 1 more to the chain will help avoid waiting on the GPU to complete internal ops before showing the next buffer.
	// Ensure that the image count does not exceed the max, unless the max is == 0
	return minUnless(this->capabilities.minImageCount + 1, this->capabilities.maxImageCount, 0);
}
