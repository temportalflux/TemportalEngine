#pragma once

#include "TemportalEnginePCH.hpp"

namespace vk
{
	struct PhysicalDeviceFeatures;
};

NS_GRAPHICS
struct SwapChainSupport;

struct PhysicalDeviceProperties
{
	/**
	 * Wrapper of `vk::PhysicalDeviceType`
	 */
	struct Type
	{
		enum class Enum : ui8
		{
			eOther = 0,
			eIntegratedGpu = 1,
			eDiscreteGpu = 2,
			eVirtualGpu = 3,
			eCpu = 4,
		};
		static std::vector<Enum> ALL;
		static std::string to_string(Enum value);
	};
	struct Extension
	{
		typedef std::string Type;
		static Type SwapChain;
		static std::vector<Type> ALL;
	};
	/**
	 * Wrapper of `vk::PhysicalDeviceType`
	 */
	struct Feature
	{
		enum class Enum : ui8
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
		static std::vector<Enum> ALL;
		static std::string to_string(Enum value);
		static bool hasFeature(vk::PhysicalDeviceFeatures const *features, Enum type);
		static bool enableFeature(vk::PhysicalDeviceFeatures *features, Enum type);
	};
};

struct QueueFamily
{
	enum class Enum : ui8
	{
		eGraphics,
		ePresentation,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct SwapChainSupportType
{
	enum class Enum : ui8
	{
		eHasAnySurfaceFormat,
		eHasAnyPresentationMode,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
	static bool hasSupport(SwapChainSupport *support, Enum type);
};

struct FilterMode
{
	// Mirrors vk::Filter
	enum class Enum : ui8
	{
		Nearest = 0,
		Linear = 1,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct SamplerAddressMode
{
	// Mirrors vk::SamplerAddressMode
	enum class Enum : ui8
	{
		Repeat = 0,
		MirroredRepeat = 1,
		ClampToEdge = 2,
		ClampToBorder = 3,
		MirrorClampToEdge = 4,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct BorderColor
{
	// Mirrors vk::BorderColor
	enum class Enum
	{
		BlackTransparentFloat = 0,
		BlackTransparentInt = 1,
		BlackOpaqueFloat = 2,
		BlackOpaqueInt = 3,
		WhiteOpaqueFloat = 4,
		WhiteOpaqueInt = 5,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct CompareOp
{
	// Mirrors vk::CompareOp
	enum class Enum
	{
		Never = 0,
		Less = 1,
		Equal = 2,
		LessOrEqual = 3,
		Greater = 4,
		NotEqual = 5,
		GreaterOrEqual = 6,
		Always = 7,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct SamplerLODMode
{
	// Mirrors vk::SamplerMipmapMode
	enum class Enum
	{
		Nearest = 0,
		Linear = 1,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct FrontFace
{
	// Mirrors vk::FrontFace
	enum class Enum
	{
		eCounterClockwise = 0,
		eClockwise = 1,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct BlendOperation
{
	// Mirrors vk::BlendOp
	enum class Enum
	{
		eAdd = 0,
		eSubtract = 1,
		eReverseSubtract = 2,
		eMin = 3,
		eMax = 4,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
	static std::string to_display_string(Enum value);
};

struct BlendFactor
{
	// Mirrors vk::BlendFactor
	enum class Enum
	{
		eZero = 0,
		eOne = 1,
		eSrcColor = 2,
		eOneMinusSrcColor = 3,
		eDstColor = 4,
		eOneMinusDstColor = 5,
		eSrcAlpha = 6,
		eOneMinusSrcAlpha = 7,
		eDstAlpha = 8,
		eOneMinusDstAlpha = 9,
		eConstantColor = 10,
		eOneMinusConstantColor = 11,
		eConstantAlpha = 12,
		eOneMinusConstantAlpha = 13,
		eSrcAlphaSaturate = 14,
		eSrc1Color = 15,
		eOneMinusSrc1Color = 16,
		eSrc1Alpha = 17,
		eOneMinusSrc1Alpha = 18,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
	static std::string to_display_string(Enum value);
};

struct ColorComponent
{
	// Mirrors vk::ColorComponentFlagBits
	enum class Enum
	{
		eR = 0x00000001,
		eG = 0x00000002,
		eB = 0x00000004,
		eA = 0x00000008,
	};
	static std::vector<Enum> ALL;
	static char to_char(Enum value);
	static std::string toFlagString(std::unordered_set<Enum> const& flags);
	static std::string to_string(Enum value);
};

struct DescriptorType
{
	// Mirrors vk::DescriptorType
	enum class Enum
	{
		eSampler = 0,
		eCombinedImageSampler = 1,
		eSampledImage = 2,
		eStorageImage = 3,
		eUniformTexelBuffer = 4,
		eStorageTexelBuffer = 5,
		eUniformBuffer = 6,
		eStorageBuffer = 7,
		eUniformBufferDynamic = 8,
		eStorageBufferDynamic = 9,
		eInputAttachment = 10,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct ShaderStage
{
	// Mirrors vk::ShaderStageFlagBits
	enum class Enum
	{
		eVertex = 0x00000001,
		eTessellationControl = 0x00000002,
		eTessellationEvaluation = 0x00000004,
		eGeometry = 0x00000008,
		eFragment = 0x00000010,
		eCompute = 0x00000020,
		eAllGraphics = 0x0000001F,
		eRaygenKHR = 0x00000100,
		eAnyHitKHR = 0x00000200,
		eClosestHitKHR = 0x00000400,
		eMissKHR = 0x00000800,
		eIntersectionKHR = 0x00001000,
		eCallableKHR = 0x00002000,
		eTaskNV = 0x00000040,
		eMeshNV = 0x00000080,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

/**
 * Custom enum which indicates a substitution for the vk::ImageFormat.
 */
struct ImageFormatReferenceType
{
	enum class Enum
	{
		/**
		 * Treated as the vk::ImageFormat of the SwapChain Image.
		 */
		Viewport,
		/**
		 * Treated as the vk::ImageFormat of the depth buffer image.
		 */
		Depth,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct SampleCount
{
	// Mirrors vk::SampleCountFlagBits
	enum class Enum
	{
		e1 = 0x00000001,
		e2 = 0x00000002,
		e4 = 0x00000004,
		e8 = 0x00000008,
		e16 = 0x00000010,
		e32 = 0x00000020,
		e64 = 0x00000040,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct AttachmentLoadOp
{
	// Mirrors vk::AttachmentLoadOp
	enum class Enum
	{
		eLoad = 0,
		eClear = 1,
		eDontCare = 2,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct AttachmentStoreOp
{
	// Mirrors vk::AttachmentStoreOp
	enum class Enum
	{
		eStore = 0,
		eDontCare = 1,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct PipelineStage
{
	// Mirrors vk::PipelineStageFlagBits
	enum class Enum
	{
		eTopOfPipe = 0x00000001,
		eDrawIndirect = 0x00000002,
		eVertexInput = 0x00000004,
		eVertexShader = 0x00000008,
		eTessellationControlShader = 0x00000010,
		eTessellationEvaluationShader = 0x00000020,
		eGeometryShader = 0x00000040,
		eFragmentShader = 0x00000080,
		eEarlyFragmentTests = 0x00000100,
		eLateFragmentTests = 0x00000200,
		eColorAttachmentOutput = 0x00000400,
		eComputeShader = 0x00000800,
		eTransfer = 0x00001000,
		eBottomOfPipe = 0x00002000,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct PrimitiveTopology
{
	// Mirrors vk::PrimitiveTopology
	enum class Enum
	{
		ePointList = 0,
		eLineList = 1,
		eLineStrip = 2,
		eTriangleList = 3,
		eTriangleStrip = 4,
		eTriangleFan = 5,
		eLineListWithAdjacency = 6,
		eLineStripWithAdjacency = 7,
		eTriangleListWithAdjacency = 8,
		eTriangleStripWithAdjacency = 9,
		ePatchList = 10,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

struct Access
{
	// Mirrors vk::AccessFlagBits
	enum class Enum
	{
		eIndirectCommandRead = 0x00000001,
		eIndexRead = 0x00000002,
		eVertexAttributeRead = 0x00000004,
		eUniformRead = 0x00000008,
		eInputAttachmentRead = 0x00000010,
		eShaderRead = 0x00000020,
		eShaderWrite = 0x00000040,
		eColorAttachmentRead = 0x00000080,
		eColorAttachmentWrite = 0x00000100,
		eDepthStencilAttachmentRead = 0x00000200,
		eDepthStencilAttachmentWrite = 0x00000400,
		eTransferRead = 0x00000800,
		eTransferWrite = 0x00001000,
		eHostRead = 0x00002000,
		eHostWrite = 0x00004000,
		eMemoryRead = 0x00008000,
		eMemoryWrite = 0x00010000,
	};
	static std::vector<Enum> ALL;
	static std::string to_string(Enum value);
};

NS_END
