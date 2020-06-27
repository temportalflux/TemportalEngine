#include "graphics/types.hpp"

#include "graphics/SwapChainSupport.hpp"

#include <vulkan/vulkan.hpp>

using namespace graphics;

#pragma region Device Type

std::vector<PhysicalDeviceProperties::Type::Enum> PhysicalDeviceProperties::Type::ALL = {
	Enum::eIntegratedGpu,
	Enum::eDiscreteGpu,
	Enum::eVirtualGpu,
	Enum::eCpu,
	Enum::eOther,
};

std::string PhysicalDeviceProperties::Type::to_string(Enum value)
{
	return vk::to_string((vk::PhysicalDeviceType)value);
}

#pragma endregion

#pragma region Device Extensions

PhysicalDeviceProperties::Extension::Type PhysicalDeviceProperties::Extension::SwapChain = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
std::vector<PhysicalDeviceProperties::Extension::Type> PhysicalDeviceProperties::Extension::ALL = {
	Extension::SwapChain,
};

#pragma endregion

#pragma region Device Features

std::vector<PhysicalDeviceProperties::Feature::Enum> PhysicalDeviceProperties::Feature::ALL = {
	Enum::RobustBufferAccess,
	Enum::FullDrawIndex,
	Enum::ImageCubeArray,
	Enum::IndependentBlend,
	Enum::GeometryShader,
	Enum::TessellationShader,
	Enum::SampleRateShading,
	Enum::DualSrcBlend,
	Enum::LogicOp,
	Enum::MultiDrawIndirect,
	Enum::DrawIndirectFirstInstance,
	Enum::DepthClamp,
	Enum::DepthBiasClamp,
	Enum::FillModeNonSolid,
	Enum::DepthBounds,
	Enum::WideLines,
	Enum::LargePoints,
	Enum::AlphaToOne,
	Enum::MultiViewport,
	Enum::SamplerAnisotropy,
	Enum::TextureCompressionETC2,
	Enum::TextureCompressionASTC_LDR,
	Enum::TextureCompressionBC,
	Enum::OcclusionQueryPrecise,
	Enum::PipelineStatisticsQuery,
	Enum::VertexPipelineStoresAndAtomics,
	Enum::FragmentStoresAndAtomics,
	Enum::ShaderTessellationAndGeometryPointSize,
	Enum::ShaderImageGatherExtended,
	Enum::ShaderStorageImageExtendedFormats,
	Enum::ShaderStorageImageMultisample,
	Enum::ShaderStorageImageReadWithoutFormat,
	Enum::ShaderStorageImageWriteWithoutFormat,
	Enum::ShaderUniformBufferArrayDynamicIndexing,
	Enum::ShaderSampledImageArrayDynamicIndexing,
	Enum::ShaderStorageBufferArrayDynamicIndexing,
	Enum::ShaderStorageImageArrayDynamicIndexing,
	Enum::ShaderClipDistance,
	Enum::ShaderCullDistance,
	Enum::ShaderFloat64,
	Enum::ShaderInt64,
	Enum::ShaderInt16,
	Enum::ShaderResourceResidency,
	Enum::ShaderResourceMinLod,
	Enum::SparseBinding,
	Enum::SparseResidencyBuffer,
	Enum::SparseResidencyImage2D,
	Enum::SparseResidencyImage3D,
	Enum::SparseResidency2Samples,
	Enum::SparseResidency4Samples,
	Enum::SparseResidency8Samples,
	Enum::SparseResidency16Samples,
	Enum::SparseResidencyAliased,
	Enum::VariableMultisampleRate,
};

std::string PhysicalDeviceProperties::Feature::to_string(Enum value)
{
	switch (value)
	{
	case Enum::RobustBufferAccess: return "Robust Buffer Access";
	case Enum::FullDrawIndex: return "Full Draw Index";
	case Enum::ImageCubeArray: return "Image Cube Array";
	case Enum::IndependentBlend: return "Independent Blend";
	case Enum::GeometryShader: return "Geometry Shader";
	case Enum::TessellationShader: return "Tessellation Shader";
	case Enum::SampleRateShading: return "Sample Rate Shading";
	case Enum::DualSrcBlend: return "Dual Src Blend";
	case Enum::LogicOp: return "Logic Op";
	case Enum::MultiDrawIndirect: return "Multidraw Indirect";
	case Enum::DrawIndirectFirstInstance: return "Draw Indirect First Instance";
	case Enum::DepthClamp: return "Depth Clamp";
	case Enum::DepthBiasClamp: return "Depth Bias Clamp";
	case Enum::FillModeNonSolid: return "Fill Mode Nonsolid";
	case Enum::DepthBounds: return "Depth Bounds";
	case Enum::WideLines: return "Wide Lines";
	case Enum::LargePoints: return "Large Points";
	case Enum::AlphaToOne: return "Alpha To One";
	case Enum::MultiViewport: return "Multiviewport";
	case Enum::SamplerAnisotropy: return "Sampler Anisotropy";
	case Enum::TextureCompressionETC2: return "Texture Compression ETC2";
	case Enum::TextureCompressionASTC_LDR: return "Textuer Compression ASTC_LDR";
	case Enum::TextureCompressionBC: return "Texture Compression BC";
	case Enum::OcclusionQueryPrecise: return "Occlusion Query Precise";
	case Enum::PipelineStatisticsQuery: return "Pipeline Statistics Query";
	case Enum::VertexPipelineStoresAndAtomics: return "Vertex Pipeline Stores and Atomics";
	case Enum::FragmentStoresAndAtomics: return "Fragment Stores and Atomics";
	case Enum::ShaderTessellationAndGeometryPointSize: return "Shader, Tessellation, and Geometry Point Size";
	case Enum::ShaderImageGatherExtended: return "Shader Image Gather Extended";
	case Enum::ShaderStorageImageExtendedFormats: return "Shader Storage Image Extended Formats";
	case Enum::ShaderStorageImageMultisample: return "Shader Storage Image Multisample";
	case Enum::ShaderStorageImageReadWithoutFormat: return "Shader Storage Image Read Without Format";
	case Enum::ShaderStorageImageWriteWithoutFormat: return "Shader Storage Image Write Without Format";
	case Enum::ShaderUniformBufferArrayDynamicIndexing: return "Shader Uniform Buffer Array Dynamic Indexing";
	case Enum::ShaderSampledImageArrayDynamicIndexing: return "Shader Sampled Image Array Dyhnamic Indexing";
	case Enum::ShaderStorageBufferArrayDynamicIndexing: return "Shader Storage Buffer Array Dynamic Instancing";
	case Enum::ShaderStorageImageArrayDynamicIndexing: return "Shader Storage Image Array Dynamic Instancing";
	case Enum::ShaderClipDistance: return "Shader Clip Distance";
	case Enum::ShaderCullDistance: return "Shader Cull Distance";
	case Enum::ShaderFloat64: return "Shader Float 64";
	case Enum::ShaderInt64: return "Shader Int 64";
	case Enum::ShaderInt16: return "Shader Int 16";
	case Enum::ShaderResourceResidency: return "Shader Resource Residency";
	case Enum::ShaderResourceMinLod: return "Shader Resource Min LOD";
	case Enum::SparseBinding: return "Sparse Binding";
	case Enum::SparseResidencyBuffer: return "Sparse Residency Buffer";
	case Enum::SparseResidencyImage2D: return "Sparse Residency Image 2D";
	case Enum::SparseResidencyImage3D: return "Sparse Residency Image 3D";
	case Enum::SparseResidency2Samples: return "Sparse Residency 2 Samples";
	case Enum::SparseResidency4Samples: return "Sparse Residency 4 Samples";
	case Enum::SparseResidency8Samples: return "Sparse Residency 8 Samples";
	case Enum::SparseResidency16Samples: return "Sparse Residency 16 Samples";
	case Enum::SparseResidencyAliased: return "Sparse Residency Aliased";
	case Enum::VariableMultisampleRate: return "Variable Multisample Rate";

	}
	return "invalid";
}

bool PhysicalDeviceProperties::Feature::hasFeature(vk::PhysicalDeviceFeatures const *features, Enum type)
{
	switch (type)
	{
	case PhysicalDeviceProperties::Feature::Enum::RobustBufferAccess: return features->robustBufferAccess;
	case PhysicalDeviceProperties::Feature::Enum::FullDrawIndex: return features->fullDrawIndexUint32;
	case PhysicalDeviceProperties::Feature::Enum::ImageCubeArray: return features->imageCubeArray;
	case PhysicalDeviceProperties::Feature::Enum::IndependentBlend: return features->independentBlend;
	case PhysicalDeviceProperties::Feature::Enum::GeometryShader: return features->geometryShader;
	case PhysicalDeviceProperties::Feature::Enum::TessellationShader: return features->tessellationShader;
	case PhysicalDeviceProperties::Feature::Enum::SampleRateShading: return features->sampleRateShading;
	case PhysicalDeviceProperties::Feature::Enum::DualSrcBlend: return features->dualSrcBlend;
	case PhysicalDeviceProperties::Feature::Enum::LogicOp: return features->logicOp;
	case PhysicalDeviceProperties::Feature::Enum::MultiDrawIndirect: return features->multiDrawIndirect;
	case PhysicalDeviceProperties::Feature::Enum::DrawIndirectFirstInstance: return features->drawIndirectFirstInstance;
	case PhysicalDeviceProperties::Feature::Enum::DepthClamp: return features->depthClamp;
	case PhysicalDeviceProperties::Feature::Enum::DepthBiasClamp: return features->depthBiasClamp;
	case PhysicalDeviceProperties::Feature::Enum::FillModeNonSolid: return features->fillModeNonSolid;
	case PhysicalDeviceProperties::Feature::Enum::DepthBounds: return features->depthBounds;
	case PhysicalDeviceProperties::Feature::Enum::WideLines: return features->wideLines;
	case PhysicalDeviceProperties::Feature::Enum::LargePoints: return features->largePoints;
	case PhysicalDeviceProperties::Feature::Enum::AlphaToOne: return features->alphaToOne;
	case PhysicalDeviceProperties::Feature::Enum::MultiViewport: return features->multiViewport;
	case PhysicalDeviceProperties::Feature::Enum::SamplerAnisotropy: return features->samplerAnisotropy;
	case PhysicalDeviceProperties::Feature::Enum::TextureCompressionETC2: return features->textureCompressionETC2;
	case PhysicalDeviceProperties::Feature::Enum::TextureCompressionASTC_LDR: return features->textureCompressionASTC_LDR;
	case PhysicalDeviceProperties::Feature::Enum::TextureCompressionBC: return features->textureCompressionBC;
	case PhysicalDeviceProperties::Feature::Enum::OcclusionQueryPrecise: return features->occlusionQueryPrecise;
	case PhysicalDeviceProperties::Feature::Enum::PipelineStatisticsQuery: return features->pipelineStatisticsQuery;
	case PhysicalDeviceProperties::Feature::Enum::VertexPipelineStoresAndAtomics: return features->vertexPipelineStoresAndAtomics;
	case PhysicalDeviceProperties::Feature::Enum::FragmentStoresAndAtomics: return features->fragmentStoresAndAtomics;
	case PhysicalDeviceProperties::Feature::Enum::ShaderTessellationAndGeometryPointSize: return features->shaderTessellationAndGeometryPointSize;
	case PhysicalDeviceProperties::Feature::Enum::ShaderImageGatherExtended: return features->shaderImageGatherExtended;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageExtendedFormats: return features->shaderStorageImageExtendedFormats;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageMultisample: return features->shaderStorageImageMultisample;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageReadWithoutFormat: return features->shaderStorageImageReadWithoutFormat;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageWriteWithoutFormat: return features->shaderStorageImageWriteWithoutFormat;
	case PhysicalDeviceProperties::Feature::Enum::ShaderUniformBufferArrayDynamicIndexing: return features->shaderUniformBufferArrayDynamicIndexing;
	case PhysicalDeviceProperties::Feature::Enum::ShaderSampledImageArrayDynamicIndexing: return features->shaderSampledImageArrayDynamicIndexing;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageBufferArrayDynamicIndexing: return features->shaderStorageBufferArrayDynamicIndexing;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageArrayDynamicIndexing: return features->shaderStorageImageArrayDynamicIndexing;
	case PhysicalDeviceProperties::Feature::Enum::ShaderClipDistance: return features->shaderClipDistance;
	case PhysicalDeviceProperties::Feature::Enum::ShaderCullDistance: return features->shaderCullDistance;
	case PhysicalDeviceProperties::Feature::Enum::ShaderFloat64: return features->shaderFloat64;
	case PhysicalDeviceProperties::Feature::Enum::ShaderInt64: return features->shaderInt64;
	case PhysicalDeviceProperties::Feature::Enum::ShaderInt16: return features->shaderInt16;
	case PhysicalDeviceProperties::Feature::Enum::ShaderResourceResidency: return features->shaderResourceResidency;
	case PhysicalDeviceProperties::Feature::Enum::ShaderResourceMinLod: return features->shaderResourceMinLod;
	case PhysicalDeviceProperties::Feature::Enum::SparseBinding: return features->sparseBinding;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyBuffer: return features->sparseResidencyBuffer;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyImage2D: return features->sparseResidencyImage2D;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyImage3D: return features->sparseResidencyImage3D;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency2Samples: return features->sparseResidency2Samples;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency4Samples: return features->sparseResidency4Samples;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency8Samples: return features->sparseResidency8Samples;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency16Samples: return features->sparseResidency16Samples;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyAliased: return features->sparseResidencyAliased;
	case PhysicalDeviceProperties::Feature::Enum::VariableMultisampleRate: return features->variableMultisampleRate;
	}
	return false;
}

bool PhysicalDeviceProperties::Feature::enableFeature(vk::PhysicalDeviceFeatures *features, Enum type)
{
	switch (type)
	{
	case PhysicalDeviceProperties::Feature::Enum::RobustBufferAccess: features->robustBufferAccess = true; break;
	case PhysicalDeviceProperties::Feature::Enum::FullDrawIndex: features->fullDrawIndexUint32 = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ImageCubeArray: features->imageCubeArray = true; break;
	case PhysicalDeviceProperties::Feature::Enum::IndependentBlend: features->independentBlend = true; break;
	case PhysicalDeviceProperties::Feature::Enum::GeometryShader: features->geometryShader = true; break;
	case PhysicalDeviceProperties::Feature::Enum::TessellationShader: features->tessellationShader = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SampleRateShading: features->sampleRateShading = true; break;
	case PhysicalDeviceProperties::Feature::Enum::DualSrcBlend: features->dualSrcBlend = true; break;
	case PhysicalDeviceProperties::Feature::Enum::LogicOp: features->logicOp = true; break;
	case PhysicalDeviceProperties::Feature::Enum::MultiDrawIndirect: features->multiDrawIndirect = true; break;
	case PhysicalDeviceProperties::Feature::Enum::DrawIndirectFirstInstance: features->drawIndirectFirstInstance = true; break;
	case PhysicalDeviceProperties::Feature::Enum::DepthClamp: features->depthClamp = true; break;
	case PhysicalDeviceProperties::Feature::Enum::DepthBiasClamp: features->depthBiasClamp = true; break;
	case PhysicalDeviceProperties::Feature::Enum::FillModeNonSolid: features->fillModeNonSolid = true; break;
	case PhysicalDeviceProperties::Feature::Enum::DepthBounds: features->depthBounds = true; break;
	case PhysicalDeviceProperties::Feature::Enum::WideLines: features->wideLines = true; break;
	case PhysicalDeviceProperties::Feature::Enum::LargePoints: features->largePoints = true; break;
	case PhysicalDeviceProperties::Feature::Enum::AlphaToOne: features->alphaToOne = true; break;
	case PhysicalDeviceProperties::Feature::Enum::MultiViewport: features->multiViewport = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SamplerAnisotropy: features->samplerAnisotropy = true; break;
	case PhysicalDeviceProperties::Feature::Enum::TextureCompressionETC2: features->textureCompressionETC2 = true; break;
	case PhysicalDeviceProperties::Feature::Enum::TextureCompressionASTC_LDR: features->textureCompressionASTC_LDR = true; break;
	case PhysicalDeviceProperties::Feature::Enum::TextureCompressionBC: features->textureCompressionBC = true; break;
	case PhysicalDeviceProperties::Feature::Enum::OcclusionQueryPrecise: features->occlusionQueryPrecise = true; break;
	case PhysicalDeviceProperties::Feature::Enum::PipelineStatisticsQuery: features->pipelineStatisticsQuery = true; break;
	case PhysicalDeviceProperties::Feature::Enum::VertexPipelineStoresAndAtomics: features->vertexPipelineStoresAndAtomics = true; break;
	case PhysicalDeviceProperties::Feature::Enum::FragmentStoresAndAtomics: features->fragmentStoresAndAtomics = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderTessellationAndGeometryPointSize: features->shaderTessellationAndGeometryPointSize = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderImageGatherExtended: features->shaderImageGatherExtended = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageExtendedFormats: features->shaderStorageImageExtendedFormats = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageMultisample: features->shaderStorageImageMultisample = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageReadWithoutFormat: features->shaderStorageImageReadWithoutFormat = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageWriteWithoutFormat: features->shaderStorageImageWriteWithoutFormat = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderUniformBufferArrayDynamicIndexing: features->shaderUniformBufferArrayDynamicIndexing = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderSampledImageArrayDynamicIndexing: features->shaderSampledImageArrayDynamicIndexing = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageBufferArrayDynamicIndexing: features->shaderStorageBufferArrayDynamicIndexing = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderStorageImageArrayDynamicIndexing: features->shaderStorageImageArrayDynamicIndexing = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderClipDistance: features->shaderClipDistance = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderCullDistance: features->shaderCullDistance = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderFloat64: features->shaderFloat64 = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderInt64: features->shaderInt64 = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderInt16: features->shaderInt16 = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderResourceResidency: features->shaderResourceResidency = true; break;
	case PhysicalDeviceProperties::Feature::Enum::ShaderResourceMinLod: features->shaderResourceMinLod = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseBinding: features->sparseBinding = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyBuffer: features->sparseResidencyBuffer = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyImage2D: features->sparseResidencyImage2D = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyImage3D: features->sparseResidencyImage3D = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency2Samples: features->sparseResidency2Samples = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency4Samples: features->sparseResidency4Samples = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency8Samples: features->sparseResidency8Samples = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidency16Samples: features->sparseResidency16Samples = true; break;
	case PhysicalDeviceProperties::Feature::Enum::SparseResidencyAliased: features->sparseResidencyAliased = true; break;
	case PhysicalDeviceProperties::Feature::Enum::VariableMultisampleRate: features->variableMultisampleRate = true; break;
	}
	return false;
}

#pragma endregion

#pragma region Queue Families

std::vector<QueueFamily::Enum> QueueFamily::ALL = {
	Enum::eGraphics,
	Enum::ePresentation,
};

std::string QueueFamily::to_string(Enum value)
{
	switch (value)
	{
	case Enum::eGraphics: return "Graphics";
	case Enum::ePresentation: return "Presentation";
	}
	return "invalid";
}

#pragma endregion

#pragma region Swap Chain

std::vector<SwapChainSupportType::Enum> SwapChainSupportType::ALL = {
	Enum::eHasAnySurfaceFormat,
	Enum::eHasAnyPresentationMode,
};

std::string SwapChainSupportType::to_string(Enum value)
{
	switch (value)
	{
	case Enum::eHasAnySurfaceFormat: return "Any Surface Format";
	case Enum::eHasAnyPresentationMode: return "Any Presentation Mode";
	}
	return "invalid";
}

bool SwapChainSupportType::hasSupport(SwapChainSupport *support, Enum type)
{
	switch (type)
	{
	case Enum::eHasAnySurfaceFormat: return !support->surfaceFormats.empty();
	case Enum::eHasAnyPresentationMode: return !support->presentationModes.empty();
	}
	return false;
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

