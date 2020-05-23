#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/SwapChainSupport.hpp"

#include <optional>
#include <set>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS

enum class PhysicalDeviceFeature
{
	RobustBufferAccess,
	FullDrawIndex,
	ImageCubeArray,
	IndependentBlend,
	GeometryShader,
	TessellationShader,
	SampleRateShading,
	DualSrcBlend,
	LogicOp,
	MultiDrawIndirect,
	DrawIndirectFirstInstance,
	DepthClamp,
	DepthBiasClamp,
	FillModeNonSolid,
	DepthBounds,
	WideLines,
	LargePoints,
	AlphaToOne,
	MultiViewport,
	SamplerAnisotropy,
	TextureCompressionETC2,
	TextureCompressionASTC_LDR,
	TextureCompressionBC,
	OcclusionQueryPrecise,
	PipelineStatisticsQuery,
	VertexPipelineStoresAndAtomics,
	FragmentStoresAndAtomics,
	ShaderTessellationAndGeometryPointSize,
	ShaderImageGatherExtended,
	ShaderStorageImageExtendedFormats,
	ShaderStorageImageMultisample,
	ShaderStorageImageReadWithoutFormat,
	ShaderStorageImageWriteWithoutFormat,
	ShaderUniformBufferArrayDynamicIndexing,
	ShaderSampledImageArrayDynamicIndexing,
	ShaderStorageBufferArrayDynamicIndexing,
	ShaderStorageImageArrayDynamicIndexing,
	ShaderClipDistance,
	ShaderCullDistance,
	ShaderFloat64,
	ShaderInt64,
	ShaderInt16,
	ShaderResourceResidency,
	ShaderResourceMinLod,
	SparseBinding,
	SparseResidencyBuffer,
	SparseResidencyImage2D,
	SparseResidencyImage3D,
	SparseResidency2Samples,
	SparseResidency4Samples,
	SparseResidency8Samples,
	SparseResidency16Samples,
	SparseResidencyAliased,
	VariableMultisampleRate,
};

class PhysicalDevicePreference
{
private:
	typedef std::optional<ui8> IndividualScore;
	typedef std::optional<ui32> TotalScore;

	struct Preference
	{
		/**
			How important this preferences is compared to others.
			If this value is `std::nullopt`, then the preferences is required
			and any gpus/physical devices which do not meet this preferences
			are ignored.
		*/
		IndividualScore score;
	};

	struct PreferenceDeviceType : Preference
	{
		vk::PhysicalDeviceType type;
	};

	struct PreferenceExtension : Preference
	{
		std::string extensionName;
	};

	struct PreferenceFeature : Preference
	{
		PhysicalDeviceFeature feature;
	};

	struct PreferenceQueueFamily : Preference
	{
		QueueFamily queueFamily;
	};

	struct PreferenceSwapChain : Preference
	{
		enum class Type
		{
			eHasAnySurfaceFormat,
			eHasAnyPresentationMode,
		};
		Type supportType;
	};

	bool isSwapChainSupported(SwapChainSupport const &support, PreferenceSwapChain::Type type) const;

public:
	PhysicalDevicePreference() = default;

	TotalScore scoreDevice(vk::PhysicalDevice const &device, vk::SurfaceKHR const &surface) const;

private:
	std::set<PreferenceDeviceType> mDeviceType;
	std::set<PreferenceExtension> mDeviceExtensions;
	std::set<PreferenceFeature> mFeatures;
	std::set<PreferenceQueueFamily> mQueueFamilies;
	std::set<PreferenceSwapChain> mSwapChain;

};

NS_END
