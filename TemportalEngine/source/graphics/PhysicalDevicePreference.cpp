#include "graphics/PhysicalDevicePreference.hpp"

#include "graphics/PhysicalDevice.hpp"

#include <unordered_set>

using namespace graphics;

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaDeviceType(vk::PhysicalDeviceType deviceType, IndividualScore score)
{
	mDeviceType.insert({ score, deviceType });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaQueueFamily(QueueFamily queueFamily, IndividualScore score)
{
	mQueueFamilies.insert({ score, queueFamily });
	return *this;
}

QueueFamilyGroup findQueueFamilies(vk::PhysicalDevice const &device, vk::SurfaceKHR const &surface)
{
	auto group = QueueFamilyGroup();
	auto familyProps = device.getQueueFamilyProperties();
	ui32 idxQueue = 0;
	for (auto& queueFamily : familyProps)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			group.idxGraphicsQueue = idxQueue;
		}

		if (device.getSurfaceSupportKHR(idxQueue, surface))
		{
			group.idxPresentationQueue = idxQueue;
		}

		if (group.hasFoundAllQueues()) break;
		idxQueue++;
	}
	return group;
}

bool doesDeviceIncludeFeatureType(vk::PhysicalDeviceFeatures const &features, PhysicalDeviceFeature type)
{
	switch (type)
	{
	case PhysicalDeviceFeature::RobustBufferAccess: return features.robustBufferAccess;
	case PhysicalDeviceFeature::FullDrawIndex: return features.fullDrawIndexUint32;
	case PhysicalDeviceFeature::ImageCubeArray: return features.imageCubeArray;
	case PhysicalDeviceFeature::IndependentBlend: return features.independentBlend;
	case PhysicalDeviceFeature::GeometryShader: return features.geometryShader;
	case PhysicalDeviceFeature::TessellationShader: return features.tessellationShader;
	case PhysicalDeviceFeature::SampleRateShading: return features.sampleRateShading;
	case PhysicalDeviceFeature::DualSrcBlend: return features.dualSrcBlend;
	case PhysicalDeviceFeature::LogicOp: return features.logicOp;
	case PhysicalDeviceFeature::MultiDrawIndirect: return features.multiDrawIndirect;
	case PhysicalDeviceFeature::DrawIndirectFirstInstance: return features.drawIndirectFirstInstance;
	case PhysicalDeviceFeature::DepthClamp: return features.depthClamp;
	case PhysicalDeviceFeature::DepthBiasClamp: return features.depthBiasClamp;
	case PhysicalDeviceFeature::FillModeNonSolid: return features.fillModeNonSolid;
	case PhysicalDeviceFeature::DepthBounds: return features.depthBounds;
	case PhysicalDeviceFeature::WideLines: return features.wideLines;
	case PhysicalDeviceFeature::LargePoints: return features.largePoints;
	case PhysicalDeviceFeature::AlphaToOne: return features.alphaToOne;
	case PhysicalDeviceFeature::MultiViewport: return features.multiViewport;
	case PhysicalDeviceFeature::SamplerAnisotropy: return features.samplerAnisotropy;
	case PhysicalDeviceFeature::TextureCompressionETC2: return features.textureCompressionETC2;
	case PhysicalDeviceFeature::TextureCompressionASTC_LDR: return features.textureCompressionASTC_LDR;
	case PhysicalDeviceFeature::TextureCompressionBC: return features.textureCompressionBC;
	case PhysicalDeviceFeature::OcclusionQueryPrecise: return features.occlusionQueryPrecise;
	case PhysicalDeviceFeature::PipelineStatisticsQuery: return features.pipelineStatisticsQuery;
	case PhysicalDeviceFeature::VertexPipelineStoresAndAtomics: return features.vertexPipelineStoresAndAtomics;
	case PhysicalDeviceFeature::FragmentStoresAndAtomics: return features.fragmentStoresAndAtomics;
	case PhysicalDeviceFeature::ShaderTessellationAndGeometryPointSize: return features.shaderTessellationAndGeometryPointSize;
	case PhysicalDeviceFeature::ShaderImageGatherExtended: return features.shaderImageGatherExtended;
	case PhysicalDeviceFeature::ShaderStorageImageExtendedFormats: return features.shaderStorageImageExtendedFormats;
	case PhysicalDeviceFeature::ShaderStorageImageMultisample: return features.shaderStorageImageMultisample;
	case PhysicalDeviceFeature::ShaderStorageImageReadWithoutFormat: return features.shaderStorageImageReadWithoutFormat;
	case PhysicalDeviceFeature::ShaderStorageImageWriteWithoutFormat: return features.shaderStorageImageWriteWithoutFormat;
	case PhysicalDeviceFeature::ShaderUniformBufferArrayDynamicIndexing: return features.shaderUniformBufferArrayDynamicIndexing;
	case PhysicalDeviceFeature::ShaderSampledImageArrayDynamicIndexing: return features.shaderSampledImageArrayDynamicIndexing;
	case PhysicalDeviceFeature::ShaderStorageBufferArrayDynamicIndexing: return features.shaderStorageBufferArrayDynamicIndexing;
	case PhysicalDeviceFeature::ShaderStorageImageArrayDynamicIndexing: return features.shaderStorageImageArrayDynamicIndexing;
	case PhysicalDeviceFeature::ShaderClipDistance: return features.shaderClipDistance;
	case PhysicalDeviceFeature::ShaderCullDistance: return features.shaderCullDistance;
	case PhysicalDeviceFeature::ShaderFloat64: return features.shaderFloat64;
	case PhysicalDeviceFeature::ShaderInt64: return features.shaderInt64;
	case PhysicalDeviceFeature::ShaderInt16: return features.shaderInt16;
	case PhysicalDeviceFeature::ShaderResourceResidency: return features.shaderResourceResidency;
	case PhysicalDeviceFeature::ShaderResourceMinLod: return features.shaderResourceMinLod;
	case PhysicalDeviceFeature::SparseBinding: return features.sparseBinding;
	case PhysicalDeviceFeature::SparseResidencyBuffer: return features.sparseResidencyBuffer;
	case PhysicalDeviceFeature::SparseResidencyImage2D: return features.sparseResidencyImage2D;
	case PhysicalDeviceFeature::SparseResidencyImage3D: return features.sparseResidencyImage3D;
	case PhysicalDeviceFeature::SparseResidency2Samples: return features.sparseResidency2Samples;
	case PhysicalDeviceFeature::SparseResidency4Samples: return features.sparseResidency4Samples;
	case PhysicalDeviceFeature::SparseResidency8Samples: return features.sparseResidency8Samples;
	case PhysicalDeviceFeature::SparseResidency16Samples: return features.sparseResidency16Samples;
	case PhysicalDeviceFeature::SparseResidencyAliased: return features.sparseResidencyAliased;
	case PhysicalDeviceFeature::VariableMultisampleRate: return features.variableMultisampleRate;
	}
	return false;
}

bool PhysicalDevicePreference::isSwapChainSupported(SwapChainSupport const &support, PreferenceSwapChain::Type type) const
{
	switch (type)
	{
	case PreferenceSwapChain::Type::eHasAnySurfaceFormat: return !support.surfaceFormats.empty();
	case PreferenceSwapChain::Type::eHasAnyPresentationMode: return !support.presentationModes.empty();
	}
	return false;
}

PhysicalDevicePreference::TotalScore PhysicalDevicePreference::scoreDevice(graphics::PhysicalDevice const *pDevice) const
{
	// Defines if the scoring should examine all preferences, even if required ones have not been found (prevents returning early).
	bool bExamineAllPreferences = false;
	// True if all required preferences have been found.
	bool bFoundAllReqiuredPreferences = true;
	// The total score of the device.
	ui32 score = 0;

	// Updates the `score` and `bFoundAllReqiuredPreferences` based on if a preference is supported.
	// Returns `bExamineAllPreferences` if the preference is required and is not supported.
	// Otherwise returns true (required and supported or regardless if it is supported or not).
	auto scorePreference = [&](Preference pref, bool bIsSupported) {
		bool bIsRequired = !pref.score.has_value();
		if (bIsRequired && !bIsSupported)
		{
			// Extension is required and not found. Mark device as not applicable and early out if desired.
			bFoundAllReqiuredPreferences = false;
			return bExamineAllPreferences;
		}
		else if (!bIsRequired && bIsSupported)
		{
			score += pref.score.value();
		}
		return true;
	};

	auto deviceProperties = pDevice->getProperties();
	for (auto& prefDeviceType : this->mDeviceType)
	{
		if (!scorePreference(prefDeviceType,
			deviceProperties.deviceType == prefDeviceType.type
		))
		{
			return std::nullopt;
		}
	}

	// Determine if all required extensions are supported, and adding the score of those that are optional to `score`.
	auto supportedExtensions = pDevice->getSupportedExtensionNames();
	for (auto& prefExtension : mDeviceExtensions)
	{
		if (!scorePreference(prefExtension,
			supportedExtensions.count(prefExtension.extensionName) != 0
		))
		{
			return std::nullopt;
		}
	}

	auto features = pDevice->getFeatures();
	for (auto& prefFeature : this->mFeatures)
	{
		if (!scorePreference(prefFeature,
			doesDeviceIncludeFeatureType(features, prefFeature.feature)
		))
		{
			return std::nullopt;
		}
	}

	auto supportedQueueFamilyGroup = pDevice->queryQueueFamilyGroup();
	for (auto& prefQueueFamily : this->mQueueFamilies)
	{
		if (!scorePreference(prefQueueFamily,
			supportedQueueFamilyGroup.hasQueueFamily(prefQueueFamily.queueFamily)
		))
		{
			return std::nullopt;
		}
	}

	auto swapChainSupport = pDevice->querySwapChainSupport();
	for (auto& prefSwapChain : this->mSwapChain)
	{
		if (!scorePreference(prefSwapChain,
			this->isSwapChainSupported(swapChainSupport, prefSwapChain.supportType)
		))
		{
			return std::nullopt;
		}
	}

	if (bExamineAllPreferences && !bFoundAllReqiuredPreferences)
	{
		return std::nullopt;
	}
	return score;
}
