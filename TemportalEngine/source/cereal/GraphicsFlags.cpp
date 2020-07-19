#include "cereal/GraphicsFlags.hpp"

#include <bitset>

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, vk::ShaderStageFlagBits const &value)
{
	return std::bitset<32>((ui32 const&)value).to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, vk::ShaderStageFlagBits &value, std::string const& str)
{
	value = (vk::ShaderStageFlagBits)std::bitset<32>(str).to_ulong();
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, vk::ShaderStageFlagBits const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, vk::ShaderStageFlagBits &value, ui32 const& i)
{
	value = (vk::ShaderStageFlagBits)i;
}

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, vk::ColorComponentFlags const &value)
{
	return std::bitset<4>((ui32 const&)value).to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, vk::ColorComponentFlags &value, std::string const& str)
{
	value = (vk::ColorComponentFlags)std::bitset<4>(str).to_ulong();
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, vk::ColorComponentFlags const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, vk::ColorComponentFlags &value, ui32 const& i)
{
	value = (vk::ColorComponentFlags)i;
}
