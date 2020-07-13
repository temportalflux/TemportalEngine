#include "graphics/DeviceObject.hpp"

using namespace graphics;

void DeviceObject::setDevice(std::weak_ptr<GraphicsDevice> device)
{
	this->mpDevice = device;
}

std::shared_ptr<GraphicsDevice> DeviceObject::device() const
{
	return this->mpDevice.lock();
}

void DeviceObject::destroy()
{
	this->invalidate();
	this->resetConfiguration();
	this->mpDevice.reset();
}
