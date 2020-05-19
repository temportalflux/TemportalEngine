#include "render/Renderer.hpp"

#include <map>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for UINT32_MAX
#include <fstream>

#include "Engine.hpp"
#include "ExecutableInfo.hpp"
#include "types/math.h"

using namespace render;

#define LogRenderer(cate, ...) DeclareLog("Renderer").log(cate, __VA_ARGS__);
#define LogVulkan(cate, ...) DeclareLog("Vulkan").log(cate, __VA_ARGS__);

#ifndef NDEBUG
VkDebugUtilsMessageSeverityFlagBitsEXT MIN_SEVERITY_TO_LOG = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
#else
VkDebugUtilsMessageSeverityFlagBitsEXT MIN_SEVERITY_TO_LOG = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif

static VKAPI_ATTR ui32 VKAPI_CALL LogVulkanOutput(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	if (severity >= MIN_SEVERITY_TO_LOG)
	{
		LogVulkan(logging::ECategory::LOGINFO, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

static std::optional<std::vector<char>> readBinary(std::string const &filePath)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		return std::nullopt;
	}

	uSize fileSize = (uSize)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

Renderer::Renderer(
	utility::SExecutableInfo const *const appInfo,
	utility::SExecutableInfo const *const engineInfo,
	ui32 width, ui32 height,
	std::vector<const char*> extensions,
	FuncCreateSurface createSurface
)
	: maRequiredExtensionsSDL(extensions)
	, mCurrentFrame(0)
{

	// Extensions ---------------------------------------------------------------

	fetchAvailableExtensions();

	// Instance -----------------------------------------------------------------

	mAppInstance = createInstance(appInfo, engineInfo);

#ifdef RENDERER_USE_VALIDATION_LAYERS
	setupVulkanMessenger();
#endif

	// Surface ------------------------------------------------------------------

	auto rawInstance = (VkInstance)mAppInstance.get();
	VkSurfaceKHR rawSurface;
	if (!createSurface(&rawInstance, &rawSurface))
	{
		LogRenderer(logging::ECategory::LOGERROR, "Failed to create an SDL KHR surface for Vulkan");
		return;
	}
	mSurface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(rawSurface));

	// Physical Device ----------------------------------------------------------

	auto optPhysicalDevice = pickPhysicalDevice();
	if (!optPhysicalDevice.has_value())
	{
		LogRenderer(logging::ECategory::LOGERROR, "Failed to find a physical device for Vulkan");
		return;
	}
	mPhysicalDevice = optPhysicalDevice.value();
	
	auto physDeviceProps = mPhysicalDevice.getProperties();
	LogRenderer(logging::ECategory::LOGINFO,
		"Loading Vulkan with physical device:\n"
		"\t%s named %s\n"
		"\tDriver Version: %i.%i.%i\n"
		"\tApi Version: %i.%i.%i\n",
		vk::to_string(physDeviceProps.deviceType).c_str(),
		physDeviceProps.deviceName,
		TE_GET_MAJOR_VERSION(physDeviceProps.driverVersion),
		TE_GET_MINOR_VERSION(physDeviceProps.driverVersion),
		TE_GET_PATCH_VERSION(physDeviceProps.driverVersion),
		TE_GET_MAJOR_VERSION(physDeviceProps.apiVersion),
		TE_GET_MINOR_VERSION(physDeviceProps.apiVersion),
		TE_GET_PATCH_VERSION(physDeviceProps.apiVersion)
	);

	// Logical Device -----------------------------------------------------------

	auto queueFamilies = this->findQueueFamilies(this->mPhysicalDevice);
	f32 graphicsQueuePriority = 1.0f;
	
	auto optLogicalDevice = this->createLogicalDevice(queueFamilies, &graphicsQueuePriority);
	if (!optLogicalDevice.has_value())
	{
		LogRenderer(logging::ECategory::LOGERROR, "Failed to create a logical device for Vulkan");
		return;
	}
	mLogicalDevice.swap(optLogicalDevice.value());

	LogRenderer(logging::ECategory::LOGINFO,
		"Created logical device for %s",
		physDeviceProps.deviceName
	);

	// Queues -----------------------------------------------------------

	mQueueGraphics = mLogicalDevice.get().getQueue(queueFamilies.idxGraphicsQueue.value(), /*queueIndex*/ 0);
	mQueuePresentation = mLogicalDevice.get().getQueue(queueFamilies.idxPresentationQueue.value(), /*queueIndex*/ 0);
	
	// Swap Chain ---------------------------------------------------------------

	mSwapChainResolution = vk::Extent2D(width, height);
	mSwapChain = createSwapchain(mSwapChainResolution, mSwapChainImageFormat);
	mSwapChainImages = mLogicalDevice->getSwapchainImagesKHR(mSwapChain.get());
	
	LogRenderer(logging::ECategory::LOGINFO,
		"Created Vulkan SwapChain with %i images at %ix%i",
		mSwapChainImages.size(),
		mSwapChainResolution.width,
		mSwapChainResolution.height
	);

	// Image Views ---------------------------------------------------------------

	instantiateImageViews();
	mRenderPass = createRenderPass();
	mPipeline = createGraphicsPipeline();
	initializeFrameBuffers();


	// Command Pool -------------------------------------------------------------

	mCommandPool = createCommandPool();
	initializeCommandBuffers();

	// Draw Frame prep -------------------------------------------------------------
	
	createSyncObjects();

}

Renderer::~Renderer()
{

}

void Renderer::fetchAvailableExtensions()
{
	auto availableExtensions = vk::enumerateInstanceExtensionProperties();
	LogRenderer(logging::ECategory::LOGDEBUG, "Available Vulkan extensions:");
	for (const auto& extension : availableExtensions)
	{
		LogRenderer(logging::ECategory::LOGDEBUG, "%s", extension.extensionName);
	}
}

std::vector<const char*> Renderer::getRequiredExtensions() const
{
	auto exts = maRequiredExtensionsSDL;
#ifdef RENDERER_USE_VALIDATION_LAYERS
	exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	return exts;
}

vk::UniqueInstance Renderer::createInstance(utility::SExecutableInfo const *const appInfo, utility::SExecutableInfo const *const engineInfo)
{
	mpApplicationInfo->pApplicationName = appInfo->title;
	mpApplicationInfo->applicationVersion = appInfo->version;
	mpApplicationInfo->pEngineName = engineInfo->title;
	mpApplicationInfo->engineVersion = engineInfo->version;
	mpApplicationInfo->apiVersion = VK_API_VERSION_1_1;

	// One day perhaps use https://en.cppreference.com/w/cpp/utility/format/format in a macro to get the string equivalent of a SemanticVersion
	LogRenderer(logging::ECategory::LOGINFO, "Initializing Vulkan v%i.%i.%i with %s Application (v%i.%i.%i) on %s Engine (v%i.%i.%i)",
		TE_GET_MAJOR_VERSION(VK_API_VERSION_1_1), TE_GET_MINOR_VERSION(VK_API_VERSION_1_1), TE_GET_PATCH_VERSION(VK_API_VERSION_1_1),
		appInfo->title,
		TE_GET_MAJOR_VERSION(appInfo->version), TE_GET_MINOR_VERSION(appInfo->version), TE_GET_PATCH_VERSION(appInfo->version),
		engineInfo->title,
		TE_GET_MAJOR_VERSION(engineInfo->version), TE_GET_MINOR_VERSION(engineInfo->version), TE_GET_PATCH_VERSION(engineInfo->version)
	);

	mpInstanceInfo->pApplicationInfo = mpApplicationInfo;

	const auto requiredExtensions = this->getRequiredExtensions();
	mpInstanceInfo->setEnabledExtensionCount((ui32)requiredExtensions.size());
	mpInstanceInfo->setPpEnabledExtensionNames(requiredExtensions.data());

	if (checkValidationLayerSupport())
	{
		mpInstanceInfo->setEnabledLayerCount((ui32)mValidationLayers.size());
		mpInstanceInfo->setPpEnabledLayerNames(mValidationLayers.data());
	}
#ifdef RENDERER_USE_VALIDATION_LAYERS
	else
	{
		LogRenderer(logging::ECategory::LOGERROR, "Vulkan validation layers requested, but there are none available.");
	}
#else
	else
	{
		mpInstanceInfo->setEnabledLayerCount(0);
	}
#endif

	// TODO: Route allocator callback through memory manager
	return vk::createInstanceUnique(*mpInstanceInfo);
}

bool Renderer::checkValidationLayerSupport() const
{
#ifndef RENDERER_USE_VALIDATION_LAYERS
	return false;
#else
	for (const auto& availableLayer : vk::enumerateInstanceLayerProperties())
	{
		for (const auto& desiredLayer : mValidationLayers)
		{
			if (strcmp(availableLayer.layerName, desiredLayer) == 0)
			{
				return true;
			}
		}
	}
	return false;
#endif
}

void Renderer::setupVulkanMessenger()
{
	auto info = vk::DebugUtilsMessengerCreateInfoEXT(
		{},
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		&LogVulkanOutput
	);
	vk::Instance inst = this->mAppInstance.get();
	VkInstance tmp = (VkInstance)inst;
	vk::DispatchLoaderDynamic dldi(tmp, vkGetInstanceProcAddr);
	auto messenger = this->mAppInstance->createDebugUtilsMessengerEXTUnique(info, nullptr, dldi);

}

std::optional<vk::PhysicalDevice> Renderer::pickPhysicalDevice()
{
	auto physicalDevices = this->mAppInstance->enumeratePhysicalDevices();

	if (physicalDevices.size() == 0) {
		LogRenderer(logging::ECategory::LOGERROR, "Failed to find any GPUs with Vulkan support!");
		return std::nullopt;
	}

	size_t deviceIndex = 0;
	std::multimap<ui8, vk::PhysicalDevice> candidates;
	for (const auto& physicalDevice : physicalDevices) {
		auto suitabilityScore = this->getPhysicalDeviceSuitabilityScore(physicalDevice);
		if (suitabilityScore.has_value())
		{
			candidates.insert(std::make_pair(suitabilityScore.value(), physicalDevice));
		}
	}
	// begin at the end of the sorted map to get the device with the highest store
	if (candidates.size() > 0)
	{
		return candidates.rbegin()->second;
	}
	else
	{
		LogRenderer(logging::ECategory::LOGERROR, "Failed to find a suitable GPU/physical device for vulkan renderer");
		return std::nullopt;
	}
}

std::optional<ui8> Renderer::getPhysicalDeviceSuitabilityScore(vk::PhysicalDevice const &device) const
{
	// Requires geometry shaders
	auto bSupportsGeometryShaders = device.getFeatures().geometryShader;
	if (!bSupportsGeometryShaders) return std::nullopt;

	auto availableQueues = this->findQueueFamilies(device);
	if (!availableQueues.hasFoundAllQueues()) return std::nullopt;

	if (!this->checkDeviceExtensionSupport(device)) return std::nullopt;

	auto swapChainSupport = this->querySwapChainSupport(device, mSurface);
	bool bSwapChainIsAdequate = !swapChainSupport.surfaceFormats.empty() && !swapChainSupport.presentationModes.empty();
	if (!bSwapChainIsAdequate) return std::nullopt;

	ui8 score = 0;
	
	// Prefer dedicated GPUs
	auto bIsDedicatedGPU = device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
	score += bIsDedicatedGPU ? 1000 : 0;

	return score;
}

bool Renderer::checkDeviceExtensionSupport(vk::PhysicalDevice const &device) const
{
	auto availableExtensions = device.enumerateDeviceExtensionProperties();
	std::set<std::string> requiredExtensions(mDeviceExtensionNames.begin(), mDeviceExtensionNames.end());
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

Renderer::QueueFamilyIndicies Renderer::findQueueFamilies(vk::PhysicalDevice const &device) const
{
	QueueFamilyIndicies indicies;

	auto familyProps = device.getQueueFamilyProperties();
	ui32 idxQueue = 0;
	for (auto& queueFamily : familyProps)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indicies.idxGraphicsQueue = idxQueue;
		}

		if (device.getSurfaceSupportKHR(idxQueue, mSurface.get()))
		{
			indicies.idxPresentationQueue = idxQueue;
		}		 

		if (indicies.hasFoundAllQueues()) break;
		idxQueue++;
	}

	return indicies;
}

std::optional<vk::UniqueDevice> Renderer::createLogicalDevice(QueueFamilyIndicies const &queueFamilies, f32 const *graphicsQueuePriority)
{
	
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
	for (const auto& uniqueQueueFamily : queueFamilies.uniqueQueues())
	{
		queueCreateInfo.push_back(vk::DeviceQueueCreateInfo()
			.setQueueCount(1)
			.setQueueFamilyIndex(queueFamilies.idxGraphicsQueue.value())
			.setPQueuePriorities(graphicsQueuePriority)
		);
	}

	auto deviceFeatures = vk::PhysicalDeviceFeatures();
	// STUB: Will need to eventually define this
	
	auto logicalDeviceInfo = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount((ui32)queueCreateInfo.size())
		.setPQueueCreateInfos(queueCreateInfo.data())
		.setPEnabledFeatures(&deviceFeatures)
		.setEnabledExtensionCount((ui32)mDeviceExtensionNames.size())
		.setPpEnabledExtensionNames(mDeviceExtensionNames.data())
		;

	#ifdef RENDERER_USE_VALIDATION_LAYERS
	logicalDeviceInfo.setEnabledLayerCount((ui32)mValidationLayers.size());
	logicalDeviceInfo.setPpEnabledLayerNames(mValidationLayers.data());
	#endif
	
	return mPhysicalDevice.createDeviceUnique(logicalDeviceInfo);
}

Renderer::SwapChainSupport Renderer::querySwapChainSupport(vk::PhysicalDevice const &device, vk::UniqueSurfaceKHR const &surface) const
{
	SwapChainSupport info;
	info.capabilities = device.getSurfaceCapabilitiesKHR(surface.get());
	info.surfaceFormats = device.getSurfaceFormatsKHR(surface.get());
	info.presentationModes = device.getSurfacePresentModesKHR(surface.get());
	return info;
}

vk::SurfaceFormatKHR const & chooseSurfaceSwapChainFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats)
{
	for (const auto& format : availableFormats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}
	return availableFormats[0];
}

vk::PresentModeKHR chooseSurfaceSwapChainPresentationMode(std::vector<vk::PresentModeKHR> const &availableModes)
{
	for (const auto& mode : availableModes) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}
	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSurfaceSwapChainExtent(vk::SurfaceCapabilitiesKHR const &capabilities, vk::Extent2D resolution)
{
	if (capabilities.currentExtent.width != UINT32_MAX) return capabilities.currentExtent;
	resolution.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, resolution.width));
	resolution.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, resolution.height));
	return resolution;
}

vk::UniqueSwapchainKHR Renderer::createSwapchain(vk::Extent2D &resolution, vk::Format &imageFormat)
{
	auto swapChainSupport = querySwapChainSupport(mPhysicalDevice, mSurface);

	vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceSwapChainFormat(swapChainSupport.surfaceFormats);
	vk::PresentModeKHR presentMode = chooseSurfaceSwapChainPresentationMode(swapChainSupport.presentationModes);
	resolution = chooseSurfaceSwapChainExtent(swapChainSupport.capabilities, resolution);

	imageFormat = surfaceFormat.format;

	// Adding at least 1 more to the chain will help avoid waiting on the GPU to complete internal ops before showing the next buffer.
	// Ensure that the image count does not exceed the max, unless the max is == 0
	ui32 imageCount = minUnless(swapChainSupport.capabilities.minImageCount + 1, swapChainSupport.capabilities.maxImageCount, 0);

	auto info = vk::SwapchainCreateInfoKHR()
		.setSurface(mSurface.get())
		.setMinImageCount(imageCount)
		.setImageFormat(imageFormat)
		.setImageColorSpace(surfaceFormat.colorSpace)
		.setPresentMode(presentMode)
		.setImageExtent(resolution)
		.setImageArrayLayers(1) // always 1 unless developing a stereoscopic 3D application
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setPreTransform(swapChainSupport.capabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setClipped(true)
		.setOldSwapchain(nullptr) // swap chain must be recreated if the window is resized. this is not yet handled
		;

	auto queueFamilyIndicies = this->findQueueFamilies(mPhysicalDevice);
	// if the families are different, use the concurrent image sharing (lower performance, but easier to set up)
	if (queueFamilyIndicies.idxGraphicsQueue != queueFamilyIndicies.idxPresentationQueue)
	{
		info.setImageSharingMode(vk::SharingMode::eConcurrent);
		info.setQueueFamilyIndexCount(2);
		info.setPQueueFamilyIndices(queueFamilyIndicies.allQueues().data());
	}
	else
	{
		info.setImageSharingMode(vk::SharingMode::eExclusive);
		info.setQueueFamilyIndexCount(0);
		info.setPQueueFamilyIndices(nullptr);
	}

	return mLogicalDevice->createSwapchainKHRUnique(info);
}

void Renderer::instantiateImageViews()
{
	mSwapChainImageViews.resize(mSwapChainImages.size());
	for (uSize i = 0; i < mSwapChainImageViews.size(); ++i)
	{
		auto info = vk::ImageViewCreateInfo()
			.setImage(this->mSwapChainImages[i])
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(mSwapChainImageFormat)
			.setComponents(vk::ComponentMapping(
				// defaults to the identity mapping where each of RGBA is its own channel
			))
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0).setLevelCount(1)
				.setBaseArrayLayer(0).setLayerCount(1)
			)
			;
		mSwapChainImageViews[i] = mLogicalDevice->createImageViewUnique(info);
	}
}

std::optional<vk::UniqueShaderModule> Renderer::createShaderModule(std::string const &filePath) const
{
	auto binary = readBinary(filePath + ".spv");
	if (!binary.has_value())
	{
		LogRenderer(logging::ECategory::LOGERROR, "Failed to read compiled SPIR-V shader: %s", filePath.c_str());
		return std::nullopt;
	}

	auto info = vk::ShaderModuleCreateInfo()
		.setCodeSize(binary.value().size())
		.setPCode(reinterpret_cast<ui32 const *>(binary.value().data()))
		;
	return mLogicalDevice->createShaderModuleUnique(info);
}

vk::UniqueRenderPass Renderer::createRenderPass()
{
	auto colorAttachment = vk::AttachmentDescription()
		.setFormat(mSwapChainImageFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	// Attachs an image to a property in the shader:
	// layout(location = 0) out vec4 outColor
	auto refColorAttachment = vk::AttachmentReference()
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
		// index of the attachment in the subpass attachments array
		.setAttachment(0);

	auto subpassDesc = vk::SubpassDescription()
		.setColorAttachmentCount(0)
		.setPColorAttachments(&refColorAttachment);

	auto subpassDependency = vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput).setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

	auto infoRenderPass = vk::RenderPassCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&colorAttachment)
		.setSubpassCount(1)
		.setPSubpasses(&subpassDesc)
		.setDependencyCount(1)
		.setPDependencies(&subpassDependency);

	return mLogicalDevice->createRenderPassUnique(infoRenderPass);
}

vk::UniquePipeline Renderer::createGraphicsPipeline()
{
	auto triangleVert = createShaderModule("resource/shaders/triangle.vert");
	assert(triangleVert.has_value());
	auto triangleFrag = createShaderModule("resource/shaders/triangle.frag");
	assert(triangleFrag.has_value());

	auto infoStageTriangleVert = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(triangleVert.value().get())
		.setPName("main")
		.setPSpecializationInfo(nullptr);

	auto infoStageTriangleFrag = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(triangleFrag.value().get())
		.setPName("main")
		.setPSpecializationInfo(nullptr);

	vk::PipelineShaderStageCreateInfo stages[] = { infoStageTriangleVert, infoStageTriangleFrag };

	auto infoInputVertex = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(0)
		.setPVertexBindingDescriptions(nullptr)
		.setVertexAttributeDescriptionCount(0)
		.setPVertexAttributeDescriptions(nullptr);

	auto infoAssembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(false);

	auto viewport = vk::Viewport()
		.setX(0).setY(0)
		.setWidth((f32)this->mSwapChainResolution.width).setHeight((f32)this->mSwapChainResolution.height)
		.setMinDepth(0.0f).setMaxDepth(1.0f);

	auto scissor = vk::Rect2D().setOffset({ 0, 0 }).setExtent(mSwapChainResolution);

	auto infoViewportState = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setPViewports(&viewport)
		.setScissorCount(1)
		.setPScissors(&scissor);

	auto infoRasterization = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.0f)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f);

	auto infoMultisampling = vk::PipelineMultisampleStateCreateInfo()
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setPSampleMask(nullptr);

	auto infoColorBlendAttachment = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(
			vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB
			| vk::ColorComponentFlagBits::eA
		)
		.setBlendEnable(false);

	auto infoColorBlendState = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(false)
		.setAttachmentCount(1)
		.setPAttachments(&infoColorBlendAttachment);

	vk::DynamicState dynamicStates[] = { vk::DynamicState::eViewport, vk::DynamicState::eLineWidth };
	auto infoDynamicStates = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(2)
		.setPDynamicStates(dynamicStates);

	auto infoLayout = vk::PipelineLayoutCreateInfo();

	auto layout = mLogicalDevice->createPipelineLayout(infoLayout);

	auto infoPipeline = vk::GraphicsPipelineCreateInfo()
		.setStageCount(2).setPStages(stages)
		.setPVertexInputState(&infoInputVertex)
		.setPInputAssemblyState(&infoAssembly)
		.setPViewportState(&infoViewportState)
		.setPRasterizationState(&infoRasterization)
		.setPMultisampleState(&infoMultisampling)
		.setPDepthStencilState(nullptr)
		.setPColorBlendState(&infoColorBlendState)
		.setPDynamicState(nullptr)
		.setLayout(layout)
		.setRenderPass(mRenderPass.get())
		.setSubpass(0);

	auto infoCache = vk::PipelineCacheCreateInfo();
	auto cache = mLogicalDevice->createPipelineCacheUnique(infoCache);

	return mLogicalDevice->createGraphicsPipelineUnique(cache.get(), infoPipeline);
}

void Renderer::initializeFrameBuffers()
{
	auto frameBufferCount = mSwapChainImageViews.size();
	mFrameBuffers.resize(frameBufferCount);
	for (uSize i = 0; i < frameBufferCount; ++i)
	{
		vk::ImageView imageViews[] = { mSwapChainImageViews[i].get() };
		auto infoFrameBuffer = vk::FramebufferCreateInfo()
			.setRenderPass(mRenderPass.get())
			.setAttachmentCount(1)
			.setPAttachments(imageViews)
			.setWidth(mSwapChainResolution.width)
			.setHeight(mSwapChainResolution.height)
			.setLayers(1);
		mFrameBuffers[i] = mLogicalDevice->createFramebufferUnique(infoFrameBuffer);
	}
}

vk::UniqueCommandPool Renderer::createCommandPool() const
{
	auto queueFamilies = this->findQueueFamilies(mPhysicalDevice);
	auto info = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(queueFamilies.idxGraphicsQueue.value());
	return mLogicalDevice->createCommandPoolUnique(info);
}

void Renderer::initializeCommandBuffers()
{
	ui32 bufferCount = (ui32)mFrameBuffers.size();
	auto allocInfo = vk::CommandBufferAllocateInfo()
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandPool(mCommandPool.get())
		.setCommandBufferCount(bufferCount);
	mCommandBuffers = mLogicalDevice->allocateCommandBuffersUnique(allocInfo);

	for (uSize i = 0; i < bufferCount; ++i)
	{
		mCommandBuffers[i]->begin(vk::CommandBufferBeginInfo());
		
		std::array<f32, 4U> color = { 0.0f, 0.0f, 0.0f, 1.0f };
		auto clearValue = vk::ClearValue().setColor(vk::ClearColorValue(color));
		auto infoPassBegin = vk::RenderPassBeginInfo()
			.setRenderPass(mRenderPass.get())
			.setFramebuffer(mFrameBuffers[i].get())
			.setRenderArea(vk::Rect2D()
				.setOffset({ 0, 0 })
				.setExtent(mSwapChainResolution)
			)
			.setClearValueCount(1)
			.setPClearValues(&clearValue);

		mCommandBuffers[i]->beginRenderPass(infoPassBegin, vk::SubpassContents::eInline);
		mCommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline.get());
		mCommandBuffers[i]->draw(3, 1, 0, 0);
		mCommandBuffers[i]->endRenderPass();

		mCommandBuffers[i]->end();
	}
}

void Renderer::createSyncObjects()
{
	mSemaphore_DrawPerFrame_ImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
	mSemaphore_DrawPerFrame_RenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
	mFencesInFlight.resize(MAX_FRAMES_IN_FLIGHT);
	mImagesInFlight.resize(mSwapChainImages.size(), VK_NULL_HANDLE);

	auto infoSemaphore = vk::SemaphoreCreateInfo();
	auto infoFence = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
	for (ui8 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		mSemaphore_DrawPerFrame_ImageAvailable[i] = mLogicalDevice->createSemaphoreUnique(infoSemaphore);
		mSemaphore_DrawPerFrame_RenderFinished[i] = mLogicalDevice->createSemaphoreUnique(infoSemaphore);
		mFencesInFlight[i] = mLogicalDevice->createFenceUnique(infoFence);
	}
}

void Renderer::drawFrame()
{
	mLogicalDevice->waitForFences(mFencesInFlight[mCurrentFrame].get(), /*wait for every fence in the list passed*/ true, UINT64_MAX);

	auto idxImage = mLogicalDevice->acquireNextImageKHR(
		mSwapChain.get(), UINT64_MAX,
		mSemaphore_DrawPerFrame_ImageAvailable[mCurrentFrame].get(),
		/*fence*/ {}
	);

	if (mImagesInFlight[idxImage.value] != VK_NULL_HANDLE)
	{
		mLogicalDevice->waitForFences((vk::Fence)mImagesInFlight[idxImage.value], /*wait for every fence in the list passed*/ true, UINT64_MAX);
	}
	mImagesInFlight[idxImage.value] = mFencesInFlight[mCurrentFrame].get();

	auto cmdBuffer = mCommandBuffers[idxImage.value].get();

	vk::Semaphore pendingSemaphores[] = { mSemaphore_DrawPerFrame_ImageAvailable[mCurrentFrame].get() };
	vk::PipelineStageFlags pendingStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore resultSemaphores[] = { mSemaphore_DrawPerFrame_RenderFinished[mCurrentFrame].get() };
	auto infoSubmit = vk::SubmitInfo()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(pendingSemaphores)
		.setPWaitDstStageMask(pendingStages)
		.setCommandBufferCount(1).setPCommandBuffers(&cmdBuffer)
		.setSignalSemaphoreCount(1).setPSignalSemaphores(resultSemaphores);

	mLogicalDevice->resetFences(mFencesInFlight[mCurrentFrame].get());

	mQueueGraphics.submit(infoSubmit, mFencesInFlight[mCurrentFrame].get());

	vk::SwapchainKHR swapchains[] = { mSwapChain.get() };
	auto infoPresent = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1).setPWaitSemaphores(resultSemaphores)
		.setSwapchainCount(1).setPSwapchains(swapchains)
		.setPImageIndices(&idxImage.value);
	mQueuePresentation.presentKHR(infoPresent);

	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
