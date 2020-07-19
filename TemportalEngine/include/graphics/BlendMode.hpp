#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS

struct BlendMode
{
	struct BlendComponent
	{
		vk::BlendOp operation;
		vk::BlendFactor srcFactor;
		vk::BlendFactor dstFactor;
	};
	
	vk::ColorComponentFlags writeMask;
	BlendComponent colorOp;
	BlendComponent alphaOp;
};

NS_END

namespace cereal
{
	void save(cereal::JSONOutputArchive &archive, graphics::BlendMode const &value);
	void save(cereal::JSONOutputArchive &archive, graphics::BlendMode::BlendComponent const &value);
	void load(cereal::JSONInputArchive &archive, graphics::BlendMode &value);
	void load(cereal::JSONInputArchive &archive, graphics::BlendMode::BlendComponent &value);
	void save(cereal::PortableBinaryOutputArchive &archive, graphics::BlendMode const &value);
	void save(cereal::PortableBinaryOutputArchive &archive, graphics::BlendMode::BlendComponent const &value);
	void load(cereal::PortableBinaryInputArchive &archive, graphics::BlendMode &value);
	void load(cereal::PortableBinaryInputArchive &archive, graphics::BlendMode::BlendComponent &value);
}
