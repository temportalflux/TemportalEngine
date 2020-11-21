#include "asset/RenderPassAsset.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_PROPERTY_CONTAINER(RenderPass)
DEFINE_FACTORY_ASSET_METADATA(RenderPass)

DEFINE_ASSET_CONSTRUCTORS(RenderPass)
	: mClearColor(math::Color({ 0, 0, 0, 1 }))
	, mClearDepthStencil(DepthStencil(0.0f, 0))
	, mRenderArea(graphics::Area({ 0, 0 }, { 1, 1 }))
{
}

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, RenderPass::write, cereal::JSONOutputArchive, RenderPass::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, RenderPass::read, cereal::JSONInputArchive, RenderPass::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, RenderPass::compile, cereal::PortableBinaryOutputArchive, RenderPass::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, RenderPass::decompile, cereal::PortableBinaryInputArchive, RenderPass::deserialize);
