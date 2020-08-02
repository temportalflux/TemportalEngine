#include "graphics/BlendMode.hpp"

#include "cereal/GraphicsFlags.hpp"
#include "cereal/optional.hpp"

using namespace graphics;

void cereal::save(cereal::JSONOutputArchive &archive, BlendMode const &value)
{
	archive(cereal::make_nvp("mask", value.writeMask));
	archive(cereal::make_nvp("blend", value.blend));
}

void cereal::save(cereal::JSONOutputArchive &archive, BlendMode::Operation const &value)
{
	archive(cereal::make_nvp("color", value.color));
	archive(cereal::make_nvp("alpha", value.alpha));
}

void cereal::save(cereal::JSONOutputArchive &archive, BlendMode::Component const &value)
{
	archive(cereal::make_nvp("operation", value.operation));
	archive(cereal::make_nvp("src", value.srcFactor));
	archive(cereal::make_nvp("dst", value.dstFactor));
}

void cereal::load(cereal::JSONInputArchive &archive, BlendMode &value)
{
	archive(cereal::make_nvp("mask", value.writeMask));
	archive(cereal::make_nvp("blend", value.blend));
}

void cereal::load(cereal::JSONInputArchive &archive, BlendMode::Operation &value)
{
	archive(cereal::make_nvp("color", value.color));
	archive(cereal::make_nvp("alpha", value.alpha));
}

void cereal::load(cereal::JSONInputArchive &archive, BlendMode::Component &value)
{
	archive(cereal::make_nvp("operation", value.operation));
	archive(cereal::make_nvp("src", value.srcFactor));
	archive(cereal::make_nvp("dst", value.dstFactor));
}

void cereal::save(cereal::PortableBinaryOutputArchive &archive, BlendMode const &value)
{
	archive(value.writeMask);
	archive(value.blend);
}

void cereal::save(cereal::PortableBinaryOutputArchive &archive, BlendMode::Operation const &value)
{
	archive(value.color);
	archive(value.alpha);
}

void cereal::save(cereal::PortableBinaryOutputArchive &archive, BlendMode::Component const &value)
{
	archive(value.operation);
	archive(value.srcFactor);
	archive(value.dstFactor);
}

void cereal::load(cereal::PortableBinaryInputArchive &archive, BlendMode &value)
{
	archive(value.writeMask);
	archive(value.blend);
}

void cereal::load(cereal::PortableBinaryInputArchive &archive, BlendMode::Operation &value)
{
	archive(value.color);
	archive(value.alpha);
}

void cereal::load(cereal::PortableBinaryInputArchive &archive, BlendMode::Component &value)
{
	archive(value.operation);
	archive(value.srcFactor);
	archive(value.dstFactor);
}
