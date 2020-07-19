#include "graphics/BlendMode.hpp"

#include "cereal/GraphicsFlags.hpp"

using namespace graphics;

void cereal::save(cereal::JSONOutputArchive &archive, BlendMode const &value)
{
	archive(cereal::make_nvp("mask", value.writeMask));
	archive(cereal::make_nvp("color", value.colorOp));
	archive(cereal::make_nvp("alpha", value.alphaOp));
}

void cereal::save(cereal::JSONOutputArchive &archive, BlendMode::BlendComponent const &value)
{
	archive(cereal::make_nvp("operation", value.operation));
	archive(cereal::make_nvp("src", value.srcFactor));
	archive(cereal::make_nvp("dst", value.dstFactor));
}

void cereal::load(cereal::JSONInputArchive &archive, BlendMode &value)
{
	archive(cereal::make_nvp("mask", value.writeMask));
	archive(cereal::make_nvp("color", value.colorOp));
	archive(cereal::make_nvp("alpha", value.alphaOp));
}

void cereal::load(cereal::JSONInputArchive &archive, BlendMode::BlendComponent &value)
{
	archive(cereal::make_nvp("operation", value.operation));
	archive(cereal::make_nvp("src", value.srcFactor));
	archive(cereal::make_nvp("dst", value.dstFactor));
}

void cereal::save(cereal::PortableBinaryOutputArchive &archive, BlendMode const &value)
{
	archive(value.writeMask);
	archive(value.colorOp);
	archive(value.alphaOp);
}

void cereal::save(cereal::PortableBinaryOutputArchive &archive, BlendMode::BlendComponent const &value)
{
	archive(value.operation);
	archive(value.srcFactor);
	archive(value.dstFactor);
}

void cereal::load(cereal::PortableBinaryInputArchive &archive, BlendMode &value)
{
	archive(value.writeMask);
	archive(value.colorOp);
	archive(value.alphaOp);
}

void cereal::load(cereal::PortableBinaryInputArchive &archive, BlendMode::BlendComponent &value)
{
	archive(value.operation);
	archive(value.srcFactor);
	archive(value.dstFactor);
}
