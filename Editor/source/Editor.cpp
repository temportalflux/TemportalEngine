#include "Editor.hpp"

#include "types/real.h"
#include "ExecutableInfo.hpp"
#include "version.h"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/LogicalDeviceInfo.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <time.h>

SDL_Window* GetSDLWindow(void* handle)
{
	return reinterpret_cast<SDL_Window*>(handle);
}

std::vector<const char*> Editor::VulkanValidationLayers = { "VK_LAYER_KHRONOS_validation" };

Editor::Editor()
	: LogSystem(logging::LogSystem())
{
	// TODO: Unify versions in a header/config file
	utility::SExecutableInfo appInfo = { "Editor", TE_MAKE_VERSION(0, 0, 1) };
	utility::SExecutableInfo engineInfo = { "TemportalEngine", TE_MAKE_VERSION(0, 0, 1) };
	std::string logFileName = "";
	logFileName += engineInfo.title;
	logFileName += "_";
	logFileName += appInfo.title;
	logFileName += "_";
	logFileName += logging::LogSystem::getCurrentTimeString();
	logFileName += ".log";
	this->LogSystem.open(logFileName.c_str());

	this->initializeDependencies();

	this->mVulkanInstance.setApplicationInfo(appInfo);
	this->mVulkanInstance.setEngineInfo(engineInfo);
	this->mVulkanInstance.createLogger(&this->LogSystem, /*vulkan debug*/ true);

}

Editor::~Editor()
{
	this->closeWindow();
	this->terminateDependencies();
	this->LogSystem.close();
}

bool Editor::initializeDependencies()
{
	if (!mDependencySDL->initialize()) return false;
	return true;
}

void Editor::terminateDependencies()
{
	if (mDependencySDL->isInitialized())
	{
		mDependencySDL->terminate();
	}
}

void Editor::openWindow()
{
	this->createWindow();
	this->initializeVulkan();

	mSurface = graphics::Surface(this->mpWindowHandle);
	mSurface.initialize(&this->mVulkanInstance);

	auto optPhysicalDevice = mVulkanInstance.pickPhysicalDevice(
		graphics::PhysicalDevicePreference()
			.addCriteriaQueueFamily(graphics::QueueFamily::eGraphics),
		&mSurface
	);
	if (!optPhysicalDevice.has_value())
	{
		mVulkanInstance.getLog().log(logging::ECategory::LOGERROR, "Failed to find a suitable GPU/physical device.");
		return;
	}
	mPhysicalDevice = optPhysicalDevice.value();

	static const auto logicalDeviceInfo = graphics::LogicalDeviceInfo()
		.addQueueFamily(graphics::QueueFamily::eGraphics)
		.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
		.setValidationLayers(VulkanValidationLayers);
	mLogicalDevice = mPhysicalDevice.createLogicalDevice(&logicalDeviceInfo);

	mQueues = mLogicalDevice.findQueues({ graphics::QueueFamily::eGraphics });

	i32 width, height;
	SDL_GetWindowSize(GetSDLWindow(this->mpWindowHandle), &width, &height);
	//this->createFrameBuffers(width, height);
}

void Editor::closeWindow()
{
	mSurface.releaseWindowHandle();
	this->destroyWindow();
	if (this->mVulkanInstance.isValid())
	{
		mQueues.clear();
		mLogicalDevice.invalidate();
		mPhysicalDevice.invalidate();
		mSurface.destroy(&this->mVulkanInstance);
		this->mVulkanInstance.destroy();
	}
}

void Editor::createWindow()
{
	auto flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	this->mpWindowHandle = SDL_CreateWindow("TemportalEngine Editor",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1280, 720,
		flags
	);
}

void Editor::destroyWindow()
{
	if (this->mpWindowHandle != nullptr)
	{
		SDL_DestroyWindow(GetSDLWindow(this->mpWindowHandle));
		this->mpWindowHandle = nullptr;
	}
}

std::vector<const char*> Editor::querySDLVulkanExtensions() const
{
	ui32 extCount = 0;
	std::vector<const char*> vulkanExtensionsForSDL = {};
	auto result = SDL_Vulkan_GetInstanceExtensions(GetSDLWindow(this->mpWindowHandle), &extCount, NULL);
	if (!result)
	{
		//LogWindow.log(logging::ECategory::LOGERROR, "SDL-Vulkan: Failed to get required vulkan extension count.");
	}
	else
	{
		//LogWindow.log(logging::ECategory::LOGDEBUG, "SDL-Vulkan: Found %i extensions", extCount);
		vulkanExtensionsForSDL.resize(extCount);
		if (!SDL_Vulkan_GetInstanceExtensions(GetSDLWindow(this->mpWindowHandle), &extCount, vulkanExtensionsForSDL.data()))
		{
			//LogWindow.log(logging::ECategory::LOGERROR, "Failed to get required vulkan extensions for SDL.");
		}
	}
	return vulkanExtensionsForSDL;
}

void Editor::initializeVulkan()
{
	this->mVulkanInstance.setRequiredExtensions(this->querySDLVulkanExtensions());
	this->mVulkanInstance.setValidationLayers(VulkanValidationLayers);
	this->mVulkanInstance.initialize();
}

void Editor::run()
{
	while (mIsRunning);
}

void Editor::setupVulkan()
{
	// Create descriptor pool
	{
		ui32 const poolSize = 1000;
		std::vector<vk::DescriptorType> poolTypes = {
			vk::DescriptorType::eSampler,
			vk::DescriptorType::eCombinedImageSampler,
			vk::DescriptorType::eSampledImage,
			vk::DescriptorType::eStorageImage,
			vk::DescriptorType::eUniformTexelBuffer,
			vk::DescriptorType::eStorageTexelBuffer,
			vk::DescriptorType::eUniformBuffer,
			vk::DescriptorType::eStorageBuffer,
			vk::DescriptorType::eUniformBufferDynamic,
			vk::DescriptorType::eStorageBufferDynamic,
			vk::DescriptorType::eInputAttachment,
		};
		std::vector<vk::DescriptorPoolSize> poolSizes;
		poolSizes.resize(poolTypes.size());
		for (ui32 i = 0; i < poolTypes.size(); ++i)
		{
			poolSizes[i] = vk::DescriptorPoolSize().setType(poolTypes[i]).setDescriptorCount(poolSize);
		}
		auto info = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(poolSize)
			.setPoolSizeCount((ui32)poolSizes.size())
			.setPPoolSizes(poolSizes.data());
		//mDescriptorPool = mLogicalDevice->createDescriptorPoolUnique(info);
	}
}

void Editor::cleanUpVulkan()
{
	mDescriptorPool.reset();
}

void Editor::createFrameBuffers(i32 const width, i32 const height)
{

}