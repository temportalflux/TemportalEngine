#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"
#include "utility/Flags.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS

// Photoshop Blend Modes! https://twitter.com/John_O_Really/status/1331601558224695298?s=20
struct BlendMode
{
	struct Component
	{
		graphics::BlendOperation operation;
		graphics::BlendFactor srcFactor;
		graphics::BlendFactor dstFactor;

		bool operator==(Component const& other) const
		{
			return operation == other.operation && srcFactor == other.srcFactor && dstFactor == other.dstFactor;
		}
		bool operator!=(Component const& other) const { return !(*this == other); }
	};
	struct Operation
	{
		Component color, alpha;

		bool operator==(Operation const& other) const { return color == other.color && alpha == other.alpha; }
		bool operator!=(Operation const& other) const { return !(*this == other); }
	};
	
	utility::Flags<graphics::ColorComponentFlags> writeMask;
	std::optional<Operation> blend;

	bool operator==(BlendMode const& other) const
	{
		return writeMask == other.writeMask && blend == other.blend;
	}
	bool operator!=(BlendMode const& other) const { return !(*this == other); }

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
