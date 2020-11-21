#include "cereal/GraphicsFlags.hpp"

#include <bitset>
#include <cctype>

#define LOAD_CASE(ENUM_TYPE, STR, ENUM_VALUE) if (str == STR) value = graphics::ENUM_TYPE::ENUM_VALUE

#pragma region vk Shader Stage

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

#pragma endregion

#pragma region vk Color Component Flags

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

#pragma endregion

#pragma region Color Component Flags

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, utility::Flags<graphics::ColorComponentFlags> const &value)
{
	return value.to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, utility::Flags<graphics::ColorComponentFlags> &value, std::string const& str)
{
	ui64 composite = 0;
	for (char c : str)
	{
		for (auto option : graphics::ColorComponent::ALL)
		{
			char optionChar = graphics::ColorComponent(option).to_string()[0];
			if (c == optionChar || c == std::tolower(optionChar))
			{
				composite |= ((ui64)option);
				break;
			}
		}
	}
	value.data() = composite;
}

ui64 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, utility::Flags<graphics::ColorComponentFlags> const &value)
{
	return value.data();
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, utility::Flags<graphics::ColorComponentFlags> &value, ui64 const& i)
{
	value.data() = i;
}

#pragma endregion

#pragma region Front Face

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, graphics::FrontFace const &value)
{
	return value.to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, graphics::FrontFace &value, std::string const& str)
{
	if (str == "Clockwise") value = graphics::EFrontFace::eClockwise;
	else value = graphics::EFrontFace::eCounterClockwise;
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::FrontFace const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::FrontFace &value, ui32 const& i)
{
	value = (graphics::EFrontFace)i;
}

#pragma endregion

#pragma region Blend Operation

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, graphics::BlendOperation const &value)
{
	return value.to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, graphics::BlendOperation &value, std::string const& str)
{
	if (str == "Add") value = graphics::EBlendOperation::eAdd;
	else if (str == "Subtract") value = graphics::EBlendOperation::eSubtract;
	else if (str == "ReverseSubtract") value = graphics::EBlendOperation::eReverseSubtract;
	else if (str == "Min") value = graphics::EBlendOperation::eMin;
	else if (str == "Max") value = graphics::EBlendOperation::eMax;
	else value = graphics::EBlendOperation::eAdd;
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::BlendOperation const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::BlendOperation &value, ui32 const& i)
{
	value = (graphics::EBlendOperation)i;
}

#pragma endregion

#pragma region Blend Factor

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, graphics::BlendFactor const &value)
{
	return value.to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, graphics::BlendFactor &value, std::string const& str)
{
	#define CASE(STR, ENUM_VALUE) LOAD_CASE(EBlendFactor, STR, ENUM_VALUE)
	CASE("Zero", eZero);
	else CASE("One", eOne);
	else CASE("SrcColor", eSrcColor);
	else CASE("OneMinusSrcColor", eOneMinusSrcColor);
	else CASE("DstColor", eDstColor);
	else CASE("OneMinusDstColor", eOneMinusDstColor);
	else CASE("SrcAlpha", eSrcAlpha);
	else CASE("OneMinusSrcAlpha", eOneMinusSrcAlpha);
	else CASE("DstAlpha", eDstAlpha);
	else CASE("OneMinusDstAlpha", eOneMinusDstAlpha);
	else CASE("ConstantColor", eConstantColor);
	else CASE("OneMinusConstantColor", eOneMinusConstantColor);
	else CASE("ConstantAlpha", eConstantAlpha);
	else CASE("OneMinusConstantAlpha", eOneMinusConstantAlpha);
	else CASE("SrcAlphaSaturate", eSrcAlphaSaturate);
	else CASE("Src1Color", eSrc1Color);
	else CASE("OneMinusSrc1Color", eOneMinusSrc1Color);
	else CASE("Src1Alpha", eSrc1Alpha);
	else CASE("OneMinusSrc1Alpha", eOneMinusSrc1Alpha);
	else value = graphics::EBlendFactor::eZero;
	#undef CASE
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::BlendFactor const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::BlendFactor &value, ui32 const& i)
{
	value = (graphics::EBlendFactor)i;
}

#pragma endregion

#pragma region Pipeline Stage

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, utility::Flags<graphics::PipelineStageFlags> const &value)
{
	return std::bitset<32>((ui32 const&)value.data()).to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, utility::Flags<graphics::PipelineStageFlags> &value, std::string const& i)
{
	value.data() = (ui64)std::bitset<32>(i).to_ulong();
}

ui64 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, utility::Flags<graphics::PipelineStageFlags> const &value) { return value.data(); }
void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, utility::Flags<graphics::PipelineStageFlags> &value, ui64 const& i) { value.data() = i; }

#pragma endregion

#pragma region Primitive Topology

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, graphics::PrimitiveTopology const &value)
{
	return value.to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, graphics::PrimitiveTopology &value, std::string const& str)
{
	LOAD_CASE(EPrimitiveTopology, "PointList", ePointList);
	else LOAD_CASE(EPrimitiveTopology, "LineList", eLineList);
	else LOAD_CASE(EPrimitiveTopology, "LineStrip", eLineStrip);
	else LOAD_CASE(EPrimitiveTopology, "TriangleList", eTriangleList);
	else LOAD_CASE(EPrimitiveTopology, "TriangleStrip", eTriangleStrip);
	else LOAD_CASE(EPrimitiveTopology, "TriangleFan", eTriangleFan);
	else LOAD_CASE(EPrimitiveTopology, "LineListWithAdjacency", eLineListWithAdjacency);
	else LOAD_CASE(EPrimitiveTopology, "LineStripWithAdjacency", eLineStripWithAdjacency);
	else LOAD_CASE(EPrimitiveTopology, "TriangleListWithAdjacency", eTriangleListWithAdjacency);
	else LOAD_CASE(EPrimitiveTopology, "TriangleStripWithAdjacency", eTriangleStripWithAdjacency);
	else LOAD_CASE(EPrimitiveTopology, "PatchList", ePatchList);
	else value = graphics::EPrimitiveTopology::eTriangleList;
}

ui32 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, graphics::PrimitiveTopology const &value)
{
	return (ui32)value;
}

void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, graphics::PrimitiveTopology &value, ui32 const& i)
{
	value = (graphics::EPrimitiveTopology)i;
}

#pragma endregion

#pragma region Access

std::string cereal::save_minimal(cereal::JSONOutputArchive const& archive, utility::Flags<graphics::AccessFlags> const &value)
{
	return std::bitset<32>((ui32 const&)value.data()).to_string();
}

void cereal::load_minimal(cereal::JSONInputArchive const& archive, utility::Flags<graphics::AccessFlags> &value, std::string const& i)
{
	value.data() = (ui64)std::bitset<32>(i).to_ulong();
}

ui64 cereal::save_minimal(cereal::PortableBinaryOutputArchive const& archive, utility::Flags<graphics::AccessFlags> const &value) { return value.data(); }
void cereal::load_minimal(cereal::PortableBinaryInputArchive const& archive, utility::Flags<graphics::AccessFlags> &value, ui64 const& i) { value.data() = i; }

#pragma endregion

#undef LOAD_CASE
