#include "cereal/GraphicsFlags.hpp"

#include <bitset>
#include <cctype>

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

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum> const &value)
{
	return graphics::ColorComponent::toFlagString(value.toSet(graphics::ColorComponent::ALL));
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum> &value, std::string const& str)
{
	ui64 composite = 0;
	for (char c : str)
	{
		for (auto option : graphics::ColorComponent::ALL)
		{
			char optionChar = graphics::ColorComponent::to_char(option);
			if (c == optionChar || c == std::tolower(optionChar))
			{
				composite |= ((ui64)option);
				break;
			}
		}
	}
	value.data() = composite;
}

ui64 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum> const &value)
{
	return value.data();
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, utility::Flags<graphics::ColorComponent::Enum> &value, ui64 const& i)
{
	value.data() = i;
}

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, graphics::BlendFactor::Enum const &value)
{
	return graphics::BlendFactor::to_string(value);
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, graphics::BlendFactor::Enum &value, std::string const& str)
{
	if (str == "Zero") value = graphics::BlendFactor::Enum::eZero;
	else if (str == "One") value = graphics::BlendFactor::Enum::eOne;
	else if (str == "SrcColor") value = graphics::BlendFactor::Enum::eSrcColor;
	else if (str == "OneMinusSrcColor") value = graphics::BlendFactor::Enum::eOneMinusSrcColor;
	else if (str == "DstColor") value = graphics::BlendFactor::Enum::eDstColor;
	else if (str == "OneMinusDstColor") value = graphics::BlendFactor::Enum::eOneMinusDstColor;
	else if (str == "SrcAlpha") value = graphics::BlendFactor::Enum::eSrcAlpha;
	else if (str == "OneMinusSrcAlpha") value = graphics::BlendFactor::Enum::eOneMinusSrcAlpha;
	else if (str == "DstAlpha") value = graphics::BlendFactor::Enum::eDstAlpha;
	else if (str == "OneMinusDstAlpha") value = graphics::BlendFactor::Enum::eOneMinusDstAlpha;
	else if (str == "ConstantColor") value = graphics::BlendFactor::Enum::eConstantColor;
	else if (str == "OneMinusConstantColor") value = graphics::BlendFactor::Enum::eOneMinusConstantColor;
	else if (str == "ConstantAlpha") value = graphics::BlendFactor::Enum::eConstantAlpha;
	else if (str == "OneMinusConstantAlpha") value = graphics::BlendFactor::Enum::eOneMinusConstantAlpha;
	else if (str == "SrcAlphaSaturate") value = graphics::BlendFactor::Enum::eSrcAlphaSaturate;
	else if (str == "Src1Color") value = graphics::BlendFactor::Enum::eSrc1Color;
	else if (str == "OneMinusSrc1Color") value = graphics::BlendFactor::Enum::eOneMinusSrc1Color;
	else if (str == "Src1Alpha") value = graphics::BlendFactor::Enum::eSrc1Alpha;
	else if (str == "OneMinusSrc1Alpha") value = graphics::BlendFactor::Enum::eOneMinusSrc1Alpha;
	else value = graphics::BlendFactor::Enum::eZero;
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::BlendFactor::Enum const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::BlendFactor::Enum &value, ui32 const& i)
{
	value = (graphics::BlendFactor::Enum)i;
}

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, graphics::BlendOperation::Enum const &value)
{
	return graphics::BlendOperation::to_string(value);
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, graphics::BlendOperation::Enum &value, std::string const& str)
{
	if (str == "Add") value = graphics::BlendOperation::Enum::eAdd;
	else if (str == "Subtract") value = graphics::BlendOperation::Enum::eSubtract;
	else if (str == "ReverseSubtract") value = graphics::BlendOperation::Enum::eReverseSubtract;
	else if (str == "Min") value = graphics::BlendOperation::Enum::eMin;
	else if (str == "Max") value = graphics::BlendOperation::Enum::eMax;
	else value = graphics::BlendOperation::Enum::eAdd;
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::BlendOperation::Enum const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::BlendOperation::Enum &value, ui32 const& i)
{
	value = (graphics::BlendOperation::Enum)i;
}
