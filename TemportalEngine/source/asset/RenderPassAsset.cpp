#include "asset/RenderPassAsset.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(RenderPass)


CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, RenderPass::write, cereal::JSONOutputArchive, RenderPass::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, RenderPass::read, cereal::JSONInputArchive, RenderPass::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, RenderPass::compile, cereal::PortableBinaryOutputArchive, RenderPass::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, RenderPass::decompile, cereal::PortableBinaryInputArchive, RenderPass::deserialize);
