#include "graphics/PhysicalDevice.hpp"

#include "graphics/Surface.hpp"
#include "graphics/LogicalDeviceInfo.hpp"

using namespace graphics;

PhysicalDevice::PhysicalDevice()
	: mpSurface(nullptr)
{
}

PhysicalDevice::PhysicalDevice(PhysicalDevice const &other)
	: mDevice(other.mDevice)
	, mpSurface(other.mpSurface)
{
}

PhysicalDevice::PhysicalDevice(vk::PhysicalDevice &device, graphics::Surface *const pSurface)
	: mDevice(device)
	, mpSurface(pSurface)
{
}

void PhysicalDevice::invalidate()
{
	mDevice = vk::PhysicalDevice(); // no longer valid
	mpSurface = nullptr;
}

vk::SurfaceKHR PhysicalDevice::getVulkanSurface() const
{
	assert(mpSurface != nullptr);
	return mpSurface->mSurface.get();
}

vk::PhysicalDeviceProperties const PhysicalDevice::getProperties() const
{
	return mDevice.getProperties();
}

std::unordered_set<std::string> PhysicalDevice::getSupportedExtensionNames() const
{
	std::unordered_set<std::string> supportedExtensionNames;
	for (auto& ext : mDevice.enumerateDeviceExtensionProperties())
	{
		supportedExtensionNames.insert(ext.extensionName);
	}
	return supportedExtensionNames;
}

vk::PhysicalDeviceFeatures const PhysicalDevice::getFeatures() const
{
	return mDevice.getFeatures();
}

QueueFamilyGroup PhysicalDevice::queryQueueFamilyGroup() const
{
	auto groups = QueueFamilyGroup();

	ui32 idxQueue = 0;
	for (auto& queueFamily : mDevice.getQueueFamilyProperties())
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			groups.idxGraphicsQueue = idxQueue;
		}

		if (mDevice.getSurfaceSupportKHR(idxQueue, getVulkanSurface()))
		{
			groups.idxPresentationQueue = idxQueue;
		}

		if (groups.hasFoundAllQueues()) break;
		idxQueue++;
	}

	return groups;
}

SwapChainSupport PhysicalDevice::querySwapChainSupport() const
{
	auto surface = getVulkanSurface();
	return {
		mDevice.getSurfaceCapabilitiesKHR(surface),
		mDevice.getSurfaceFormatsKHR(surface),
		mDevice.getSurfacePresentModesKHR(surface),
	};
}

LogicalDevice PhysicalDevice::createLogicalDevice(LogicalDeviceInfo const * pInfo) const
{
	auto queueFamilies = this->queryQueueFamilyGroup();
	auto queueInfo = pInfo->makeQueueInfo(&queueFamilies);
	auto queueCreateInfo = std::vector<vk::DeviceQueueCreateInfo>(queueInfo.size());
	for (uSize i = 0; i < queueInfo.size(); ++i)
		queueCreateInfo[i] = queueInfo[i].makeInfo();
	auto info = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount((ui32)queueCreateInfo.size())
		.setPQueueCreateInfos(queueCreateInfo.data())
		.setEnabledExtensionCount((ui32)pInfo->mDeviceExtensions.size())
		.setPpEnabledExtensionNames(pInfo->mDeviceExtensions.data())
		.setEnabledLayerCount((ui32)pInfo->mValidationLayers.size())
		.setPpEnabledLayerNames(pInfo->mValidationLayers.data());
	vk::UniqueDevice device = mDevice.createDeviceUnique(info);
	return LogicalDevice(this, device);
}
