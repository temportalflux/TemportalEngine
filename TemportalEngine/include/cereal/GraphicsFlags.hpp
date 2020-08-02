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

namespace cereal
{

	std::string save_minimal(cereal::JSONOutputArchive const& archive, vk::ShaderStageFlagBits const &value);
	void load_minimal(cereal::JSONInputArchive const& archive, vk::ShaderStageFlagBits &value, std::string const& str);
	ui32 save_minimal(cereal::PortableBinaryOutputArchive const& archive, vk::ShaderStageFlagBits const &value);
	void load_minimal(cereal::PortableBinaryInputArchive const& archive, vk::ShaderStageFlagBits &value, ui32 const& i);

	std::string save_minimal(cereal::JSONOutputArchive const& archive, vk::ColorComponentFlags const &value);
	void load_minimal(cereal::JSONInputArchive const& archive, vk::ColorComponentFlags &value, std::string const& str);
	ui32 save_minimal(cereal::PortableBinaryOutputArchive const& archive, vk::ColorComponentFlags const &value);
	void load_minimal(cereal::PortableBinaryInputArchive const& archive, vk::ColorComponentFlags &value, ui32 const& i);

	std::string save_minimal(cereal::JSONOutputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum> const &value);
	void load_minimal(cereal::JSONInputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum>  &value, std::string const& str);
	ui64 save_minimal(cereal::PortableBinaryOutputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum> const &value);
	void load_minimal(cereal::PortableBinaryInputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum> &value, ui64 const& i);

	std::string save_minimal(cereal::JSONOutputArchive const& archive, graphics::BlendFactor::Enum const &value);
	void load_minimal(cereal::JSONInputArchive const& archive, graphics::BlendFactor::Enum &value, std::string const& str);
	ui32 save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::BlendFactor::Enum const &value);
	void load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::BlendFactor::Enum &value, ui32 const& i);

	std::string save_minimal(cereal::JSONOutputArchive const& archive, graphics::BlendOperation::Enum const &value);
	void load_minimal(cereal::JSONInputArchive const& archive, graphics::BlendOperation::Enum &value, std::string const& str);
	ui32 save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::BlendOperation::Enum const &value);
	void load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::BlendOperation::Enum &value, ui32 const& i);

}
