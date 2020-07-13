#pragma once

#include "TemportalEnginePCH.hpp"

NS_GRAPHICS
class GraphicsDevice;

class DeviceObject
{

public:
	/**
	 * Sets the pointer to the device this object is created by.
	 */
	void setDevice(std::weak_ptr<GraphicsDevice> device);
	/**
	 * Returns a shared pointer to the device set by `setDevice`.
	 */
	std::shared_ptr<GraphicsDevice> device() const;

	/**
	 * Implemented by subclasses to initialize the Vulkan object this class represents.
	 */
	virtual void create() = 0;
	/**
	 * Returns the raw pointer to the Vulkan object.
	 * Can use `reinterpret_cast<VulkanObject*>(deviceObject.get())` to access low-level vulkan interface data.
	 */
	virtual void* get() = 0;
	/**
	 * Destroys the device object entirely, calling `invalidate` to reset the creation and also destroying any configured data.
	 */
	void destroy();
	/**
	 * Invalidates the object created by `create` but does not reset any settings.
	 */
	virtual void invalidate() = 0;
	/**
	 * Resets any configurable data that is used to create the vulkan object.
	 */
	virtual void resetConfiguration() = 0;

private:
	std::weak_ptr<GraphicsDevice> mpDevice;

};

NS_END
