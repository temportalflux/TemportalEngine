#include "graphics/DeviceObject.hpp"

using namespace graphics;

DeviceObject::DeviceObject()
{
	this->resetConfiguration();
}

void DeviceObject::setDevice(std::weak_ptr<GraphicsDevice> device)
{
	this->mpDevice = device;
}

std::weak_ptr<GraphicsDevice> DeviceObject::getDevice() const
{
	return this->mpDevice;
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
