#include "graphics/types.hpp"

#include "graphics/SwapChainSupport.hpp"

#include <vulkan/vulkan.hpp>

using namespace graphics;

#pragma region Device Type

std::vector<EPhysicalDeviceType> PhysicalDeviceType::ALL = {
	EPhysicalDeviceType::eIntegratedGpu,
	EPhysicalDeviceType::eDiscreteGpu,
	EPhysicalDeviceType::eVirtualGpu,
	EPhysicalDeviceType::eCpu,
	EPhysicalDeviceType::eOther,
};

std::string PhysicalDeviceType::to_string() const { return vk::to_string(as<vk::PhysicalDeviceType>()); }
std::string PhysicalDeviceType::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Device Extensions

PhysicalDeviceExtension::Type PhysicalDeviceExtension::ExtSwapChain = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
std::vector<PhysicalDeviceExtension::Type> PhysicalDeviceExtension::ALL = {
	PhysicalDeviceExtension::ExtSwapChain,
};

#pragma endregion

#pragma region Device Features

std::vector<EDeviceFeature> DeviceFeature::ALL = {
	EDeviceFeature::RobustBufferAccess,
	EDeviceFeature::FullDrawIndex,
	EDeviceFeature::ImageCubeArray,
	EDeviceFeature::IndependentBlend,
	EDeviceFeature::GeometryShader,
	EDeviceFeature::TessellationShader,
	EDeviceFeature::SampleRateShading,
	EDeviceFeature::DualSrcBlend,
	EDeviceFeature::LogicOp,
	EDeviceFeature::MultiDrawIndirect,
	EDeviceFeature::DrawIndirectFirstInstance,
	EDeviceFeature::DepthClamp,
	EDeviceFeature::DepthBiasClamp,
	EDeviceFeature::FillModeNonSolid,
	EDeviceFeature::DepthBounds,
	EDeviceFeature::WideLines,
	EDeviceFeature::LargePoints,
	EDeviceFeature::AlphaToOne,
	EDeviceFeature::MultiViewport,
	EDeviceFeature::SamplerAnisotropy,
	EDeviceFeature::TextureCompressionETC2,
	EDeviceFeature::TextureCompressionASTC_LDR,
	EDeviceFeature::TextureCompressionBC,
	EDeviceFeature::OcclusionQueryPrecise,
	EDeviceFeature::PipelineStatisticsQuery,
	EDeviceFeature::VertexPipelineStoresAndAtomics,
	EDeviceFeature::FragmentStoresAndAtomics,
	EDeviceFeature::ShaderTessellationAndGeometryPointSize,
	EDeviceFeature::ShaderImageGatherExtended,
	EDeviceFeature::ShaderStorageImageExtendedFormats,
	EDeviceFeature::ShaderStorageImageMultisample,
	EDeviceFeature::ShaderStorageImageReadWithoutFormat,
	EDeviceFeature::ShaderStorageImageWriteWithoutFormat,
	EDeviceFeature::ShaderUniformBufferArrayDynamicIndexing,
	EDeviceFeature::ShaderSampledImageArrayDynamicIndexing,
	EDeviceFeature::ShaderStorageBufferArrayDynamicIndexing,
	EDeviceFeature::ShaderStorageImageArrayDynamicIndexing,
	EDeviceFeature::ShaderClipDistance,
	EDeviceFeature::ShaderCullDistance,
	EDeviceFeature::ShaderFloat64,
	EDeviceFeature::ShaderInt64,
	EDeviceFeature::ShaderInt16,
	EDeviceFeature::ShaderResourceResidency,
	EDeviceFeature::ShaderResourceMinLod,
	EDeviceFeature::SparseBinding,
	EDeviceFeature::SparseResidencyBuffer,
	EDeviceFeature::SparseResidencyImage2D,
	EDeviceFeature::SparseResidencyImage3D,
	EDeviceFeature::SparseResidency2Samples,
	EDeviceFeature::SparseResidency4Samples,
	EDeviceFeature::SparseResidency8Samples,
	EDeviceFeature::SparseResidency16Samples,
	EDeviceFeature::SparseResidencyAliased,
	EDeviceFeature::VariableMultisampleRate,
};

std::string DeviceFeature::to_string() const
{
	switch (value())
	{
	case EDeviceFeature::RobustBufferAccess: return "Robust Buffer Access";
	case EDeviceFeature::FullDrawIndex: return "Full Draw Index";
	case EDeviceFeature::ImageCubeArray: return "Image Cube Array";
	case EDeviceFeature::IndependentBlend: return "Independent Blend";
	case EDeviceFeature::GeometryShader: return "Geometry Shader";
	case EDeviceFeature::TessellationShader: return "Tessellation Shader";
	case EDeviceFeature::SampleRateShading: return "Sample Rate Shading";
	case EDeviceFeature::DualSrcBlend: return "Dual Src Blend";
	case EDeviceFeature::LogicOp: return "Logic Op";
	case EDeviceFeature::MultiDrawIndirect: return "Multidraw Indirect";
	case EDeviceFeature::DrawIndirectFirstInstance: return "Draw Indirect First Instance";
	case EDeviceFeature::DepthClamp: return "Depth Clamp";
	case EDeviceFeature::DepthBiasClamp: return "Depth Bias Clamp";
	case EDeviceFeature::FillModeNonSolid: return "Fill Mode Nonsolid";
	case EDeviceFeature::DepthBounds: return "Depth Bounds";
	case EDeviceFeature::WideLines: return "Wide Lines";
	case EDeviceFeature::LargePoints: return "Large Points";
	case EDeviceFeature::AlphaToOne: return "Alpha To One";
	case EDeviceFeature::MultiViewport: return "Multiviewport";
	case EDeviceFeature::SamplerAnisotropy: return "Sampler Anisotropy";
	case EDeviceFeature::TextureCompressionETC2: return "Texture Compression ETC2";
	case EDeviceFeature::TextureCompressionASTC_LDR: return "Textuer Compression ASTC_LDR";
	case EDeviceFeature::TextureCompressionBC: return "Texture Compression BC";
	case EDeviceFeature::OcclusionQueryPrecise: return "Occlusion Query Precise";
	case EDeviceFeature::PipelineStatisticsQuery: return "Pipeline Statistics Query";
	case EDeviceFeature::VertexPipelineStoresAndAtomics: return "Vertex Pipeline Stores and Atomics";
	case EDeviceFeature::FragmentStoresAndAtomics: return "Fragment Stores and Atomics";
	case EDeviceFeature::ShaderTessellationAndGeometryPointSize: return "Shader, Tessellation, and Geometry Point Size";
	case EDeviceFeature::ShaderImageGatherExtended: return "Shader Image Gather Extended";
	case EDeviceFeature::ShaderStorageImageExtendedFormats: return "Shader Storage Image Extended Formats";
	case EDeviceFeature::ShaderStorageImageMultisample: return "Shader Storage Image Multisample";
	case EDeviceFeature::ShaderStorageImageReadWithoutFormat: return "Shader Storage Image Read Without Format";
	case EDeviceFeature::ShaderStorageImageWriteWithoutFormat: return "Shader Storage Image Write Without Format";
	case EDeviceFeature::ShaderUniformBufferArrayDynamicIndexing: return "Shader Uniform Buffer Array Dynamic Indexing";
	case EDeviceFeature::ShaderSampledImageArrayDynamicIndexing: return "Shader Sampled Image Array Dyhnamic Indexing";
	case EDeviceFeature::ShaderStorageBufferArrayDynamicIndexing: return "Shader Storage Buffer Array Dynamic Instancing";
	case EDeviceFeature::ShaderStorageImageArrayDynamicIndexing: return "Shader Storage Image Array Dynamic Instancing";
	case EDeviceFeature::ShaderClipDistance: return "Shader Clip Distance";
	case EDeviceFeature::ShaderCullDistance: return "Shader Cull Distance";
	case EDeviceFeature::ShaderFloat64: return "Shader Float 64";
	case EDeviceFeature::ShaderInt64: return "Shader Int 64";
	case EDeviceFeature::ShaderInt16: return "Shader Int 16";
	case EDeviceFeature::ShaderResourceResidency: return "Shader Resource Residency";
	case EDeviceFeature::ShaderResourceMinLod: return "Shader Resource Min LOD";
	case EDeviceFeature::SparseBinding: return "Sparse Binding";
	case EDeviceFeature::SparseResidencyBuffer: return "Sparse Residency Buffer";
	case EDeviceFeature::SparseResidencyImage2D: return "Sparse Residency Image 2D";
	case EDeviceFeature::SparseResidencyImage3D: return "Sparse Residency Image 3D";
	case EDeviceFeature::SparseResidency2Samples: return "Sparse Residency 2 Samples";
	case EDeviceFeature::SparseResidency4Samples: return "Sparse Residency 4 Samples";
	case EDeviceFeature::SparseResidency8Samples: return "Sparse Residency 8 Samples";
	case EDeviceFeature::SparseResidency16Samples: return "Sparse Residency 16 Samples";
	case EDeviceFeature::SparseResidencyAliased: return "Sparse Residency Aliased";
	case EDeviceFeature::VariableMultisampleRate: return "Variable Multisample Rate";
	default: return "invalid";
	}
}
std::string DeviceFeature::to_display_string() const { return to_string(); }

bool graphics::hasFeature(vk::PhysicalDeviceFeatures const *features, DeviceFeature type)
{
	switch (type.value())
	{
	case EDeviceFeature::RobustBufferAccess: return features->robustBufferAccess;
	case EDeviceFeature::FullDrawIndex: return features->fullDrawIndexUint32;
	case EDeviceFeature::ImageCubeArray: return features->imageCubeArray;
	case EDeviceFeature::IndependentBlend: return features->independentBlend;
	case EDeviceFeature::GeometryShader: return features->geometryShader;
	case EDeviceFeature::TessellationShader: return features->tessellationShader;
	case EDeviceFeature::SampleRateShading: return features->sampleRateShading;
	case EDeviceFeature::DualSrcBlend: return features->dualSrcBlend;
	case EDeviceFeature::LogicOp: return features->logicOp;
	case EDeviceFeature::MultiDrawIndirect: return features->multiDrawIndirect;
	case EDeviceFeature::DrawIndirectFirstInstance: return features->drawIndirectFirstInstance;
	case EDeviceFeature::DepthClamp: return features->depthClamp;
	case EDeviceFeature::DepthBiasClamp: return features->depthBiasClamp;
	case EDeviceFeature::FillModeNonSolid: return features->fillModeNonSolid;
	case EDeviceFeature::DepthBounds: return features->depthBounds;
	case EDeviceFeature::WideLines: return features->wideLines;
	case EDeviceFeature::LargePoints: return features->largePoints;
	case EDeviceFeature::AlphaToOne: return features->alphaToOne;
	case EDeviceFeature::MultiViewport: return features->multiViewport;
	case EDeviceFeature::SamplerAnisotropy: return features->samplerAnisotropy;
	case EDeviceFeature::TextureCompressionETC2: return features->textureCompressionETC2;
	case EDeviceFeature::TextureCompressionASTC_LDR: return features->textureCompressionASTC_LDR;
	case EDeviceFeature::TextureCompressionBC: return features->textureCompressionBC;
	case EDeviceFeature::OcclusionQueryPrecise: return features->occlusionQueryPrecise;
	case EDeviceFeature::PipelineStatisticsQuery: return features->pipelineStatisticsQuery;
	case EDeviceFeature::VertexPipelineStoresAndAtomics: return features->vertexPipelineStoresAndAtomics;
	case EDeviceFeature::FragmentStoresAndAtomics: return features->fragmentStoresAndAtomics;
	case EDeviceFeature::ShaderTessellationAndGeometryPointSize: return features->shaderTessellationAndGeometryPointSize;
	case EDeviceFeature::ShaderImageGatherExtended: return features->shaderImageGatherExtended;
	case EDeviceFeature::ShaderStorageImageExtendedFormats: return features->shaderStorageImageExtendedFormats;
	case EDeviceFeature::ShaderStorageImageMultisample: return features->shaderStorageImageMultisample;
	case EDeviceFeature::ShaderStorageImageReadWithoutFormat: return features->shaderStorageImageReadWithoutFormat;
	case EDeviceFeature::ShaderStorageImageWriteWithoutFormat: return features->shaderStorageImageWriteWithoutFormat;
	case EDeviceFeature::ShaderUniformBufferArrayDynamicIndexing: return features->shaderUniformBufferArrayDynamicIndexing;
	case EDeviceFeature::ShaderSampledImageArrayDynamicIndexing: return features->shaderSampledImageArrayDynamicIndexing;
	case EDeviceFeature::ShaderStorageBufferArrayDynamicIndexing: return features->shaderStorageBufferArrayDynamicIndexing;
	case EDeviceFeature::ShaderStorageImageArrayDynamicIndexing: return features->shaderStorageImageArrayDynamicIndexing;
	case EDeviceFeature::ShaderClipDistance: return features->shaderClipDistance;
	case EDeviceFeature::ShaderCullDistance: return features->shaderCullDistance;
	case EDeviceFeature::ShaderFloat64: return features->shaderFloat64;
	case EDeviceFeature::ShaderInt64: return features->shaderInt64;
	case EDeviceFeature::ShaderInt16: return features->shaderInt16;
	case EDeviceFeature::ShaderResourceResidency: return features->shaderResourceResidency;
	case EDeviceFeature::ShaderResourceMinLod: return features->shaderResourceMinLod;
	case EDeviceFeature::SparseBinding: return features->sparseBinding;
	case EDeviceFeature::SparseResidencyBuffer: return features->sparseResidencyBuffer;
	case EDeviceFeature::SparseResidencyImage2D: return features->sparseResidencyImage2D;
	case EDeviceFeature::SparseResidencyImage3D: return features->sparseResidencyImage3D;
	case EDeviceFeature::SparseResidency2Samples: return features->sparseResidency2Samples;
	case EDeviceFeature::SparseResidency4Samples: return features->sparseResidency4Samples;
	case EDeviceFeature::SparseResidency8Samples: return features->sparseResidency8Samples;
	case EDeviceFeature::SparseResidency16Samples: return features->sparseResidency16Samples;
	case EDeviceFeature::SparseResidencyAliased: return features->sparseResidencyAliased;
	case EDeviceFeature::VariableMultisampleRate: return features->variableMultisampleRate;
	}
	return false;
}

bool graphics::enableFeature(vk::PhysicalDeviceFeatures *features, DeviceFeature type)
{
	switch (type.value())
	{
	case EDeviceFeature::RobustBufferAccess: features->robustBufferAccess = true; break;
	case EDeviceFeature::FullDrawIndex: features->fullDrawIndexUint32 = true; break;
	case EDeviceFeature::ImageCubeArray: features->imageCubeArray = true; break;
	case EDeviceFeature::IndependentBlend: features->independentBlend = true; break;
	case EDeviceFeature::GeometryShader: features->geometryShader = true; break;
	case EDeviceFeature::TessellationShader: features->tessellationShader = true; break;
	case EDeviceFeature::SampleRateShading: features->sampleRateShading = true; break;
	case EDeviceFeature::DualSrcBlend: features->dualSrcBlend = true; break;
	case EDeviceFeature::LogicOp: features->logicOp = true; break;
	case EDeviceFeature::MultiDrawIndirect: features->multiDrawIndirect = true; break;
	case EDeviceFeature::DrawIndirectFirstInstance: features->drawIndirectFirstInstance = true; break;
	case EDeviceFeature::DepthClamp: features->depthClamp = true; break;
	case EDeviceFeature::DepthBiasClamp: features->depthBiasClamp = true; break;
	case EDeviceFeature::FillModeNonSolid: features->fillModeNonSolid = true; break;
	case EDeviceFeature::DepthBounds: features->depthBounds = true; break;
	case EDeviceFeature::WideLines: features->wideLines = true; break;
	case EDeviceFeature::LargePoints: features->largePoints = true; break;
	case EDeviceFeature::AlphaToOne: features->alphaToOne = true; break;
	case EDeviceFeature::MultiViewport: features->multiViewport = true; break;
	case EDeviceFeature::SamplerAnisotropy: features->samplerAnisotropy = true; break;
	case EDeviceFeature::TextureCompressionETC2: features->textureCompressionETC2 = true; break;
	case EDeviceFeature::TextureCompressionASTC_LDR: features->textureCompressionASTC_LDR = true; break;
	case EDeviceFeature::TextureCompressionBC: features->textureCompressionBC = true; break;
	case EDeviceFeature::OcclusionQueryPrecise: features->occlusionQueryPrecise = true; break;
	case EDeviceFeature::PipelineStatisticsQuery: features->pipelineStatisticsQuery = true; break;
	case EDeviceFeature::VertexPipelineStoresAndAtomics: features->vertexPipelineStoresAndAtomics = true; break;
	case EDeviceFeature::FragmentStoresAndAtomics: features->fragmentStoresAndAtomics = true; break;
	case EDeviceFeature::ShaderTessellationAndGeometryPointSize: features->shaderTessellationAndGeometryPointSize = true; break;
	case EDeviceFeature::ShaderImageGatherExtended: features->shaderImageGatherExtended = true; break;
	case EDeviceFeature::ShaderStorageImageExtendedFormats: features->shaderStorageImageExtendedFormats = true; break;
	case EDeviceFeature::ShaderStorageImageMultisample: features->shaderStorageImageMultisample = true; break;
	case EDeviceFeature::ShaderStorageImageReadWithoutFormat: features->shaderStorageImageReadWithoutFormat = true; break;
	case EDeviceFeature::ShaderStorageImageWriteWithoutFormat: features->shaderStorageImageWriteWithoutFormat = true; break;
	case EDeviceFeature::ShaderUniformBufferArrayDynamicIndexing: features->shaderUniformBufferArrayDynamicIndexing = true; break;
	case EDeviceFeature::ShaderSampledImageArrayDynamicIndexing: features->shaderSampledImageArrayDynamicIndexing = true; break;
	case EDeviceFeature::ShaderStorageBufferArrayDynamicIndexing: features->shaderStorageBufferArrayDynamicIndexing = true; break;
	case EDeviceFeature::ShaderStorageImageArrayDynamicIndexing: features->shaderStorageImageArrayDynamicIndexing = true; break;
	case EDeviceFeature::ShaderClipDistance: features->shaderClipDistance = true; break;
	case EDeviceFeature::ShaderCullDistance: features->shaderCullDistance = true; break;
	case EDeviceFeature::ShaderFloat64: features->shaderFloat64 = true; break;
	case EDeviceFeature::ShaderInt64: features->shaderInt64 = true; break;
	case EDeviceFeature::ShaderInt16: features->shaderInt16 = true; break;
	case EDeviceFeature::ShaderResourceResidency: features->shaderResourceResidency = true; break;
	case EDeviceFeature::ShaderResourceMinLod: features->shaderResourceMinLod = true; break;
	case EDeviceFeature::SparseBinding: features->sparseBinding = true; break;
	case EDeviceFeature::SparseResidencyBuffer: features->sparseResidencyBuffer = true; break;
	case EDeviceFeature::SparseResidencyImage2D: features->sparseResidencyImage2D = true; break;
	case EDeviceFeature::SparseResidencyImage3D: features->sparseResidencyImage3D = true; break;
	case EDeviceFeature::SparseResidency2Samples: features->sparseResidency2Samples = true; break;
	case EDeviceFeature::SparseResidency4Samples: features->sparseResidency4Samples = true; break;
	case EDeviceFeature::SparseResidency8Samples: features->sparseResidency8Samples = true; break;
	case EDeviceFeature::SparseResidency16Samples: features->sparseResidency16Samples = true; break;
	case EDeviceFeature::SparseResidencyAliased: features->sparseResidencyAliased = true; break;
	case EDeviceFeature::VariableMultisampleRate: features->variableMultisampleRate = true; break;
	}
	return false;
}

#pragma endregion

#pragma region Queue Families

std::vector<EQueueFamily> QueueFamily::ALL = {
	EQueueFamily::eGraphics,
	EQueueFamily::ePresentation,
};

std::string QueueFamily::to_string() const
{
	switch (value())
	{
	case EQueueFamily::eGraphics: return "Graphics";
	case EQueueFamily::ePresentation: return "Presentation";
	default: return "invalid";
	}
}
std::string QueueFamily::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Swap Chain

std::vector<ESwapChainSupport> SwapChainSupportType::ALL = {
	ESwapChainSupport::eHasAnySurfaceFormat,
	ESwapChainSupport::eHasAnyPresentationMode,
};

std::string SwapChainSupportType::to_string() const
{
	switch (value())
	{
	case ESwapChainSupport::eHasAnySurfaceFormat: return "Any Surface Format";
	case ESwapChainSupport::eHasAnyPresentationMode: return "Any Presentation Mode";
	default: return "invalid";
	}
}
std::string SwapChainSupportType::to_display_string() const { return to_string(); }

bool graphics::hasSupport(SwapChainSupport *support, SwapChainSupportType type)
{
	switch (type.value())
	{
	case ESwapChainSupport::eHasAnySurfaceFormat: return !support->surfaceFormats.empty();
	case ESwapChainSupport::eHasAnyPresentationMode: return !support->presentationModes.empty();
	default: return false;
	}
}

#pragma endregion

#pragma region FilterMode

std::vector<FilterMode::Enum> FilterMode::ALL = {
	Enum::Nearest,
	Enum::Linear,
};

std::string FilterMode::to_string(Enum value)
{
	return vk::to_string((vk::Filter)value);
}

#pragma endregion

#pragma region SamplerAddressMode

std::vector<SamplerAddressMode::Enum> SamplerAddressMode::ALL = {
	Enum::Repeat,
	Enum::MirroredRepeat,
	Enum::ClampToEdge,
	Enum::ClampToBorder,
	Enum::MirrorClampToEdge,
};

std::string SamplerAddressMode::to_string(Enum value)
{
	return vk::to_string((vk::SamplerAddressMode)value);
}

#pragma endregion

#pragma region BorderColor

std::vector<BorderColor::Enum> BorderColor::ALL = {
	Enum::BlackTransparentFloat,
	Enum::BlackTransparentInt,
	Enum::BlackOpaqueFloat,
	Enum::BlackOpaqueInt,
	Enum::WhiteOpaqueFloat,
	Enum::WhiteOpaqueInt,
};

std::string BorderColor::to_string(Enum value)
{
	return vk::to_string((vk::BorderColor)value);
}

#pragma endregion

#pragma region CompareOp

std::vector<CompareOp::Enum> CompareOp::ALL = {
	Enum::Never,
	Enum::Less,
	Enum::Equal,
	Enum::LessOrEqual,
	Enum::Greater,
	Enum::NotEqual,
	Enum::GreaterOrEqual,
	Enum::Always,
};

std::string CompareOp::to_string(Enum value)
{
	return vk::to_string((vk::CompareOp)value);
}

#pragma endregion

#pragma region SamplerLODMode

std::vector<SamplerLODMode::Enum> SamplerLODMode::ALL = {
	Enum::Nearest,
	Enum::Linear,
};

std::string SamplerLODMode::to_string(Enum value)
{
	return vk::to_string((vk::SamplerMipmapMode)value);
}

#pragma endregion

#pragma region Front Face

std::vector<EFrontFace> FrontFace::ALL = {
	EFrontFace::eCounterClockwise,
	EFrontFace::eClockwise,
};

std::string FrontFace::to_string() const { return vk::to_string(as<vk::FrontFace>()); }
std::string FrontFace::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Blend Operation

std::vector<EBlendOperation> BlendOperation::ALL = {
	EBlendOperation::eAdd,
	EBlendOperation::eSubtract,
	EBlendOperation::eReverseSubtract,
	EBlendOperation::eMin,
	EBlendOperation::eMax,
};

std::string BlendOperation::to_string() const { return vk::to_string(as<vk::BlendOp>()); }

std::string BlendOperation::to_display_string() const
{
	switch (value())
	{
	case EBlendOperation::eAdd: return "+";
	case EBlendOperation::eSubtract: return "-";
	case EBlendOperation::eReverseSubtract: return "*(-1) +";
	case EBlendOperation::eMin: return "<min>";
	case EBlendOperation::eMax: return "<max>";
	default: return "invalid";
	}
}

#pragma endregion

#pragma region Blend Factor

std::vector<EBlendFactor> BlendFactor::ALL = {
		EBlendFactor::eZero,
		EBlendFactor::eOne,
		EBlendFactor::eSrcColor,
		EBlendFactor::eOneMinusSrcColor,
		EBlendFactor::eDstColor,
		EBlendFactor::eOneMinusDstColor,
		EBlendFactor::eSrcAlpha,
		EBlendFactor::eOneMinusSrcAlpha,
		EBlendFactor::eDstAlpha,
		EBlendFactor::eOneMinusDstAlpha,
		EBlendFactor::eConstantColor,
		EBlendFactor::eOneMinusConstantColor,
		EBlendFactor::eConstantAlpha,
		EBlendFactor::eOneMinusConstantAlpha,
		EBlendFactor::eSrcAlphaSaturate,
		EBlendFactor::eSrc1Color,
		EBlendFactor::eOneMinusSrc1Color,
		EBlendFactor::eSrc1Alpha,
		EBlendFactor::eOneMinusSrc1Alpha,
};

std::string BlendFactor::to_string() const { return vk::to_string(as<vk::BlendFactor>()); }

std::string BlendFactor::to_display_string() const
{
	switch (value())
	{
	case EBlendFactor::eZero: return "0";
	case EBlendFactor::eOne: return "1";
	case EBlendFactor::eSrcColor: return "srcColor";
	case EBlendFactor::eOneMinusSrcColor: return "(1 - srcColor)";
	case EBlendFactor::eDstColor: return "dstColor";
	case EBlendFactor::eOneMinusDstColor: return "(1 - dstColor)";
	case EBlendFactor::eSrcAlpha: return "srcAlpha";
	case EBlendFactor::eOneMinusSrcAlpha: return "(1 - srcAlpha)";
	case EBlendFactor::eDstAlpha: return "dstAlpha";
	case EBlendFactor::eOneMinusDstAlpha: return "(1 - dstAlpha)";
	case EBlendFactor::eConstantColor: return "constColor";
	case EBlendFactor::eOneMinusConstantColor: return "(1 - constColor)";
	case EBlendFactor::eConstantAlpha: return "constAlpha";
	case EBlendFactor::eOneMinusConstantAlpha: return "(1 - constAlpha)";
	case EBlendFactor::eSrcAlphaSaturate: return "srcAlphaSaturate";
	case EBlendFactor::eSrc1Color: return "srcColor1";
	case EBlendFactor::eOneMinusSrc1Color: return "(1 - srcColor1)";
	case EBlendFactor::eSrc1Alpha: return "srcAlpha1";
	case EBlendFactor::eOneMinusSrc1Alpha: return "(1 - srcAlpha1)";
	default: return "invalid";
	}
}

#pragma endregion

#pragma region ColorComponent

std::vector<ColorComponentFlags> ColorComponent::ALL = {
		ColorComponentFlags::eR,
		ColorComponentFlags::eG,
		ColorComponentFlags::eB,
		ColorComponentFlags::eA,
};

std::string ColorComponent::to_string() const
{
	switch (value())
	{
	case ColorComponentFlags::eR: return "R";
	case ColorComponentFlags::eG: return "G";
	case ColorComponentFlags::eB: return "B";
	case ColorComponentFlags::eA: return "A";
	default: return "\0";
	}
}

std::string ColorComponent::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Descriptor Type

std::vector<EDescriptorType> DescriptorType::ALL = {
		EDescriptorType::eSampler,
		EDescriptorType::eCombinedImageSampler,
		EDescriptorType::eSampledImage,
		EDescriptorType::eStorageImage,
		EDescriptorType::eUniformTexelBuffer,
		EDescriptorType::eStorageTexelBuffer,
		EDescriptorType::eUniformBuffer,
		EDescriptorType::eStorageBuffer,
		EDescriptorType::eUniformBufferDynamic,
		EDescriptorType::eStorageBufferDynamic,
		EDescriptorType::eInputAttachment,
};

std::string DescriptorType::to_string() const { return vk::to_string(as<vk::DescriptorType>()); }
std::string DescriptorType::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Shader Stage

std::vector<ShaderStageFlags> ShaderStage::ALL = {
		ShaderStageFlags::eVertex,
		ShaderStageFlags::eTessellationControl,
		ShaderStageFlags::eTessellationEvaluation,
		ShaderStageFlags::eGeometry,
		ShaderStageFlags::eFragment,
		ShaderStageFlags::eCompute,
		ShaderStageFlags::eAllGraphics,
		ShaderStageFlags::eRaygenKHR,
		ShaderStageFlags::eAnyHitKHR,
		ShaderStageFlags::eClosestHitKHR,
		ShaderStageFlags::eMissKHR,
		ShaderStageFlags::eIntersectionKHR,
		ShaderStageFlags::eCallableKHR,
		ShaderStageFlags::eTaskNV,
		ShaderStageFlags::eMeshNV,
};

std::string ShaderStage::to_string() const { return vk::to_string(as<vk::ShaderStageFlagBits>()); }
std::string ShaderStage::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Image Format Reference Type

std::vector<EImageFormatCategory> ImageFormatCategory::ALL = {
	EImageFormatCategory::Viewport,
	EImageFormatCategory::Depth,
};

std::string ImageFormatCategory::to_string() const
{
	switch (value())
	{
	case EImageFormatCategory::Viewport: return "Viewport";
	case EImageFormatCategory::Depth: return "Depth";
	default: return "invalid";
	}
}
std::string ImageFormatCategory::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Sample Count

std::vector<ESampleCount> SampleCount::ALL = {
		ESampleCount::e1,
		ESampleCount::e2,
		ESampleCount::e4,
		ESampleCount::e8,
		ESampleCount::e16,
		ESampleCount::e32,
		ESampleCount::e64,
};

std::string SampleCount::to_string() const { return vk::to_string(as<vk::SampleCountFlagBits>()); }
std::string SampleCount::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Attachment Load

std::vector<EAttachmentLoadOp> AttachmentLoadOp::ALL = {
		EAttachmentLoadOp::eLoad,
		EAttachmentLoadOp::eClear,
		EAttachmentLoadOp::eDontCare,
};

std::string AttachmentLoadOp::to_string() const { return vk::to_string(as<vk::AttachmentLoadOp>()); }
std::string AttachmentLoadOp::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Attachment Store

std::vector<EAttachmentStoreOp> AttachmentStoreOp::ALL = {
		EAttachmentStoreOp::eStore,
		EAttachmentStoreOp::eDontCare,
};

std::string AttachmentStoreOp::to_string() const { return vk::to_string(as<vk::AttachmentStoreOp>()); }
std::string AttachmentStoreOp::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Pipeline Stage

std::vector<PipelineStageFlags> PipelineStage::ALL = {
		PipelineStageFlags::eTopOfPipe,
		PipelineStageFlags::eDrawIndirect,
		PipelineStageFlags::eVertexInput,
		PipelineStageFlags::eVertexShader,
		PipelineStageFlags::eTessellationControlShader,
		PipelineStageFlags::eTessellationEvaluationShader,
		PipelineStageFlags::eGeometryShader,
		PipelineStageFlags::eFragmentShader,
		PipelineStageFlags::eEarlyFragmentTests,
		PipelineStageFlags::eLateFragmentTests,
		PipelineStageFlags::eColorAttachmentOutput,
		PipelineStageFlags::eComputeShader,
		PipelineStageFlags::eTransfer,
		PipelineStageFlags::eBottomOfPipe,
};

std::string PipelineStage::to_string() const { return vk::to_string(as<vk::PipelineStageFlagBits>()); }
std::string PipelineStage::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Primitive Topology

std::vector<EPrimitiveTopology> PrimitiveTopology::ALL = {
		EPrimitiveTopology::ePointList,
		EPrimitiveTopology::eLineList,
		EPrimitiveTopology::eLineStrip,
		EPrimitiveTopology::eTriangleList,
		EPrimitiveTopology::eTriangleStrip,
		EPrimitiveTopology::eTriangleFan,
		EPrimitiveTopology::eLineListWithAdjacency,
		EPrimitiveTopology::eLineStripWithAdjacency,
		EPrimitiveTopology::eTriangleListWithAdjacency,
		EPrimitiveTopology::eTriangleStripWithAdjacency,
		EPrimitiveTopology::ePatchList,
};

std::string PrimitiveTopology::to_string() const { return vk::to_string(as<vk::PrimitiveTopology>()); }
std::string PrimitiveTopology::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Access

std::vector<AccessFlags> Access::ALL = {
		AccessFlags::eIndirectCommandRead,
		AccessFlags::eIndexRead,
		AccessFlags::eVertexAttributeRead,
		AccessFlags::eUniformRead,
		AccessFlags::eInputAttachmentRead,
		AccessFlags::eShaderRead,
		AccessFlags::eShaderWrite,
		AccessFlags::eColorAttachmentRead,
		AccessFlags::eColorAttachmentWrite,
		AccessFlags::eDepthStencilAttachmentRead,
		AccessFlags::eDepthStencilAttachmentWrite,
		AccessFlags::eTransferRead,
		AccessFlags::eTransferWrite,
		AccessFlags::eHostRead,
		AccessFlags::eHostWrite,
		AccessFlags::eMemoryRead,
		AccessFlags::eMemoryWrite,
};

std::string Access::to_string() const { return vk::to_string(as<vk::AccessFlagBits>()); }
std::string Access::to_display_string() const { return to_string(); }

#pragma endregion

#pragma region Image Layout

std::vector<EImageLayout> ImageLayout::ALL = {
	EImageLayout::eUndefined,
	EImageLayout::eGeneral,
	EImageLayout::eColorAttachmentOptimal,
	EImageLayout::eDepthStencilAttachmentOptimal,
	EImageLayout::eDepthStencilReadOnlyOptimal,
	EImageLayout::eShaderReadOnlyOptimal,
	EImageLayout::eTransferSrcOptimal,
	EImageLayout::eTransferDstOptimal,
	EImageLayout::ePreinitialized,
	EImageLayout::eDepthReadWrite,
	EImageLayout::eDepthReadOnly,
	EImageLayout::eStencilReadWrite,
	EImageLayout::eStencilReadOnly,
	EImageLayout::ePresentSrc,
	EImageLayout::eSharedPresent,
};

std::string ImageLayout::to_string() const { return vk::to_string(as<vk::ImageLayout>()); }
std::string ImageLayout::to_display_string() const { return to_string(); }

#pragma endregion
