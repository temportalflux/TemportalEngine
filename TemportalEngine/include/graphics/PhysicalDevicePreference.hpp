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
public:
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

		bool isScoreLessThan(Preference const &other) const
		{
			return !this->score.has_value() && other.score.has_value();
		}
	};

	struct PreferenceDeviceType : Preference
	{
		vk::PhysicalDeviceType type;

		bool operator<(PreferenceDeviceType const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->type, other.type);
		}
	};

	struct PreferenceExtension : Preference
	{
		std::string extensionName;

		bool operator<(PreferenceExtension const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->extensionName, other.extensionName);
		}
	};

	struct PreferenceFeature : Preference
	{
		PhysicalDeviceFeature feature;

		bool operator<(PreferenceFeature const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->feature, other.feature);
		}
	};

	struct PreferenceQueueFamily : Preference
	{
		QueueFamily queueFamily;

		bool operator<(PreferenceQueueFamily const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->queueFamily, other.queueFamily);
		}
	};

	struct PreferenceSwapChain : Preference
	{
		enum class Type
		{
			eHasAnySurfaceFormat,
			eHasAnyPresentationMode,
		};
		Type supportType;

		bool operator<(PreferenceSwapChain const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->supportType, other.supportType);
		}
	};

	bool isSwapChainSupported(SwapChainSupport const &support, PreferenceSwapChain::Type type) const;

public:
	PhysicalDevicePreference() = default;

	PhysicalDevicePreference& addCriteriaDeviceType(vk::PhysicalDeviceType deviceType, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaQueueFamily(QueueFamily queueFamily, IndividualScore score = std::nullopt);
	
	TotalScore scoreDevice(vk::PhysicalDevice const &device, vk::SurfaceKHR const &surface) const;

private:
	std::set<PreferenceDeviceType> mDeviceType;
	std::set<PreferenceExtension> mDeviceExtensions;
	std::set<PreferenceFeature> mFeatures;
	std::set<PreferenceQueueFamily> mQueueFamilies;
	std::set<PreferenceSwapChain> mSwapChain;

};

NS_END
