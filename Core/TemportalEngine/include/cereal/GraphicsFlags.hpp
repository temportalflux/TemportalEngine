#pragma once

#include "TemportalEnginePCH.hpp"

#include "utility/Flags.hpp"
#include "graphics/types.hpp"

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <vulkan/vulkan.hpp>

//CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(vk::ShaderStageFlagBits, cereal::specialization::non_member_load_save);
//CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(vk::ColorComponentFlags, cereal::specialization::non_member_load_save);

#define DECLARE_CEREALIZATION(TYPE, JSON_OUT, BINARY_OUT) \
	JSON_OUT save_minimal(cereal::JSONOutputArchive const& archive, TYPE const &value); \
	void load_minimal(cereal::JSONInputArchive const& archive, TYPE &value, JSON_OUT const& out); \
	BINARY_OUT save_minimal(cereal::PortableBinaryOutputArchive const& archive, TYPE const &value); \
	void load_minimal(cereal::PortableBinaryInputArchive const& archive, TYPE &value, BINARY_OUT const& out);

namespace cereal
{

	DECLARE_CEREALIZATION(vk::ShaderStageFlagBits, std::string, ui32)
	DECLARE_CEREALIZATION(vk::ColorComponentFlags, std::string, ui32)

	DECLARE_CEREALIZATION(graphics::PhysicalDeviceType, std::string, ui32)
	DECLARE_CEREALIZATION(graphics::DeviceFeature, ui32, ui32)
	DECLARE_CEREALIZATION(graphics::QueueFamily, std::string, ui32)
	DECLARE_CEREALIZATION(graphics::SwapChainSupportType, ui32, ui32)
	
	DECLARE_CEREALIZATION(graphics::FrontFace, std::string, ui32)
	DECLARE_CEREALIZATION(graphics::BlendOperation, std::string, ui32)
	DECLARE_CEREALIZATION(graphics::BlendFactor, std::string, ui32)
	DECLARE_CEREALIZATION(graphics::PrimitiveTopology, std::string, ui32)
	DECLARE_CEREALIZATION(utility::Flags<graphics::ColorComponentFlags>, std::string, ui64)
	DECLARE_CEREALIZATION(utility::Flags<graphics::PipelineStageFlags>, std::string, ui64)
	DECLARE_CEREALIZATION(utility::Flags<graphics::AccessFlags>, std::string, ui64)

}

#undef DECLARE_CEREALIZATION
