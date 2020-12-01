#include "asset/PipelineAsset.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_PROPERTY_CONTAINER(Pipeline)
DEFINE_FACTORY_ASSET_METADATA(Pipeline)

DEFINE_ASSET_CONSTRUCTORS(Pipeline)
	: mVertexShader(TypedAssetPath<Shader>())
	, mFragmentShader(TypedAssetPath<Shader>())
	, mViewport(graphics::Viewport({ 0, 0 }, { 1, 1 }, { 0, 1 }))
	, mScissor(graphics::Area({ 0, 0 }, { 1, 1 }))
	, mFrontFace(graphics::EFrontFace::eCounterClockwise)
	, mBlendMode(graphics::BlendMode{ utility::Flags<graphics::ColorComponentFlags>(0b1111), std::nullopt })
	, mTopology(graphics::EPrimitiveTopology::eTriangleList)
	, mLineWidth(1)
	, mDescriptorGroups({})
{

}

std::vector<AssetPath const*> Pipeline::getAssetRefs() const { return { &mVertexShader.path(), &mFragmentShader.path() }; }
std::vector<AssetPath*> Pipeline::getAssetRefs() { return { &mVertexShader.path(), &mFragmentShader.path() }; }

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Pipeline::write, cereal::JSONOutputArchive, Pipeline::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Pipeline::read, cereal::JSONInputArchive, Pipeline::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Pipeline::compile, cereal::PortableBinaryOutputArchive, Pipeline::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Pipeline::decompile, cereal::PortableBinaryInputArchive, Pipeline::deserialize);
