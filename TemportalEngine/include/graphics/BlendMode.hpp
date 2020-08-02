#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"
#include "utility/Flags.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS

struct BlendMode
{
	struct Component
	{
		graphics::BlendOperation::Enum operation;
		graphics::BlendFactor::Enum srcFactor;
		graphics::BlendFactor::Enum dstFactor;
	};
	struct Operation
	{
		Component color, alpha;
	};
	
	utility::Flags<graphics::ColorComponent::Enum> writeMask;
	std::optional<Operation> blend;
};

NS_END

namespace cereal
{
	void save(cereal::JSONOutputArchive &archive, graphics::BlendMode const &value);
	void save(cereal::JSONOutputArchive &archive, graphics::BlendMode::Operation const &value);
	void save(cereal::JSONOutputArchive &archive, graphics::BlendMode::Component const &value);
	void load(cereal::JSONInputArchive &archive, graphics::BlendMode &value);
	void load(cereal::JSONInputArchive &archive, graphics::BlendMode::Operation &value);
	void load(cereal::JSONInputArchive &archive, graphics::BlendMode::Component &value);
	void save(cereal::PortableBinaryOutputArchive &archive, graphics::BlendMode const &value);
	void save(cereal::PortableBinaryOutputArchive &archive, graphics::BlendMode::Operation const &value);
	void save(cereal::PortableBinaryOutputArchive &archive, graphics::BlendMode::Component const &value);
	void load(cereal::PortableBinaryInputArchive &archive, graphics::BlendMode &value);
	void load(cereal::PortableBinaryInputArchive &archive, graphics::BlendMode::Operation &value);
	void load(cereal::PortableBinaryInputArchive &archive, graphics::BlendMode::Component &value);
}
