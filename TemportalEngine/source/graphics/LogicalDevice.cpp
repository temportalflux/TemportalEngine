#include "graphics/LogicalDevice.hpp"

using namespace graphics;

LogicalDevice::LogicalDevice(vk::UniqueDevice &device)
{
	mDevice.swap(device);
}

void LogicalDevice::invalidate()
{
	this->mDevice.reset();
}
