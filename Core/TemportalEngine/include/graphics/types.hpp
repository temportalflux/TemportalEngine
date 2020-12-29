#pragma once

#include "TemportalEnginePCH.hpp"
#include "utility/Flags.hpp"

namespace vk
{
	struct PhysicalDeviceFeatures;
};

NS_GRAPHICS
struct SwapChainSupport;

// Mirrors vk::PhysicalDeviceType
enum class EPhysicalDeviceType : ui8
{
	eOther = 0,
	eIntegratedGpu = 1,
	eDiscreteGpu = 2,
	eVirtualGpu = 3,
	eCpu = 4,
};
typedef utility::EnumWrapper<EPhysicalDeviceType> PhysicalDeviceType;

struct PhysicalDeviceExtension
{
	typedef std::string Type;
	// Elements
	static std::vector<Type> ALL;
	static Type ExtSwapChain;
};

enum class EDeviceFeature
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
typedef utility::EnumWrapper<EDeviceFeature> DeviceFeature;
bool hasFeature(vk::PhysicalDeviceFeatures const *features, DeviceFeature type);
bool enableFeature(vk::PhysicalDeviceFeatures *features, DeviceFeature type);

enum class EQueueFamily : ui8
{
	eGraphics,
	ePresentation,
};
typedef utility::EnumWrapper<EQueueFamily> QueueFamily;

enum class ESwapChainSupport : ui8
{
	eHasAnySurfaceFormat,
	eHasAnyPresentationMode,
};
typedef utility::EnumWrapper<ESwapChainSupport> SwapChainSupportType;
bool hasSupport(SwapChainSupport *support, SwapChainSupportType type);

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

// Mirrors vk::FrontFace
enum class EFrontFace
{
	eCounterClockwise = 0,
	eClockwise = 1,
};
typedef utility::EnumWrapper<EFrontFace> FrontFace;

// Mirrors vk::BlendOp
enum class EBlendOperation
{
	eAdd = 0,
	eSubtract = 1,
	eReverseSubtract = 2,
	eMin = 3,
	eMax = 4,
};
typedef utility::EnumWrapper<EBlendOperation> BlendOperation;

// Mirrors vk::BlendFactor
enum class EBlendFactor
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
typedef utility::EnumWrapper<EBlendFactor> BlendFactor;

// Mirrors vk::ColorComponentFlagBits
enum class ColorComponentFlags : uint8_t
{
	eR = 0x00000001,
	eG = 0x00000002,
	eB = 0x00000004,
	eA = 0x00000008,
};
typedef utility::EnumWrapper<ColorComponentFlags> ColorComponent;

// Mirrors vk::DescriptorType
enum class EDescriptorType
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
typedef utility::EnumWrapper<EDescriptorType> DescriptorType;

// Mirrors vk::ShaderStageFlagBits
enum class ShaderStageFlags
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
typedef utility::EnumWrapper<ShaderStageFlags> ShaderStage;

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

// Mirrors vk::PipelineStageFlagBits
enum class PipelineStageFlags : uint64_t
{
	eTopOfPipe = 0x00000001, // 0b1
	eDrawIndirect = 0x00000002, // 0b10
	eVertexInput = 0x00000004, // 0b100
	eVertexShader = 0x00000008, // 0b1000
	eTessellationControlShader = 0x00000010, // 0b10000
	eTessellationEvaluationShader = 0x00000020, // 0b100000
	eGeometryShader = 0x00000040, // 0b1000000
	eFragmentShader = 0x00000080, // 0b10000000
	eEarlyFragmentTests = 0x00000100, // 0b100000000
	eLateFragmentTests = 0x00000200, // 0b1000000000
	eColorAttachmentOutput = 0x00000400, // 0b10000000000
	eComputeShader = 0x00000800, // 0b100000000000
	eTransfer = 0x00001000, // 0b1000000000000
	eBottomOfPipe = 0x00002000, // 0b10000000000000
};
typedef utility::EnumWrapper<PipelineStageFlags> PipelineStage;

// Mirrors vk::PrimitiveTopology
enum class EPrimitiveTopology
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
typedef utility::EnumWrapper<EPrimitiveTopology> PrimitiveTopology;

// Mirrors vk::AccessFlagBits
enum class AccessFlags : uint64_t
{
	eIndirectCommandRead = 0x00000001, // 0b1
	eIndexRead = 0x00000002, // 0b10
	eVertexAttributeRead = 0x00000004, // 0b100
	eUniformRead = 0x00000008, // 0b1000
	eInputAttachmentRead = 0x00000010, // 0b10000
	eShaderRead = 0x00000020, // 0b100000
	eShaderWrite = 0x00000040, // 0b1000000
	eColorAttachmentRead = 0x00000080, // 0b10000000
	eColorAttachmentWrite = 0x00000100, // 0b100000000
	eDepthStencilAttachmentRead = 0x00000200, // 0b1000000000
	eDepthStencilAttachmentWrite = 0x00000400, // 0b10000000000
	eTransferRead = 0x00000800, // 0b100000000000
	eTransferWrite = 0x00001000, // 0b1000000000000
	eHostRead = 0x00002000, // 0b10000000000000
	eHostWrite = 0x00004000, // 0b100000000000000
	eMemoryRead = 0x00008000, // 0b1000000000000000
	eMemoryWrite = 0x00010000, // 0b10000000000000000
};
typedef utility::EnumWrapper<AccessFlags> Access;

enum class EImageLayout : ui8
{
	eUndefined = 0,
	eGeneral = 1,
	eColorAttachmentOptimal = 2,
	eDepthStencilAttachmentOptimal = 3,
	eDepthStencilReadOnlyOptimal = 4,
	eShaderReadOnlyOptimal = 5,
	eTransferSrcOptimal = 6,
	eTransferDstOptimal = 7,
	ePreinitialized = 8,
};
typedef utility::EnumWrapper<EImageLayout> ImageLayout;

NS_END
