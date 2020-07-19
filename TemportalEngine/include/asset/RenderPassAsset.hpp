#pragma once

#include "asset/Asset.hpp"

#include "graphics/Area.hpp"
#include "cereal/list.hpp"
#include "cereal/optional.hpp"
#include "cereal/mathVector.hpp"

NS_ASSET
class Pipeline;

class RenderPass : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_STATICS("renderpass", "Render Pass", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GRAPHICS);
	DECLARE_FACTORY_ASSET_METADATA()

	RenderPass() = default;
	CREATE_NEWASSET_CONSTRUCTOR(RenderPass) {}

private:

	std::optional<math::Vector4> mClearColor;
	std::optional<std::pair<f32, ui32>> mClearDepthStencil;
	graphics::Area mRenderArea;
	std::vector<TypedAssetPath<Pipeline>> mPipelines;

#pragma region Serialization
protected:
	DECLARE_SERIALIZATION_METHOD(write, cereal::JSONOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(read, cereal::JSONInputArchive, override);
	DECLARE_SERIALIZATION_METHOD(compile, cereal::PortableBinaryOutputArchive, const override);
	DECLARE_SERIALIZATION_METHOD(decompile, cereal::PortableBinaryInputArchive, override);

	template <typename Archive>
	void serialize(Archive &archive) const
	{
		Asset::serialize(archive);
		archive(cereal::make_nvp("clearColor", this->mClearColor));
		archive(cereal::make_nvp("clearDepthStencil", this->mClearDepthStencil));
		archive(cereal::make_nvp("renderArea", this->mRenderArea));
		archive(cereal::make_nvp("pipelines", this->mPipelines));
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		Asset::deserialize(archive);
		archive(cereal::make_nvp("clearColor", this->mClearColor));
		archive(cereal::make_nvp("clearDepthStencil", this->mClearDepthStencil));
		archive(cereal::make_nvp("renderArea", this->mRenderArea));
		archive(cereal::make_nvp("pipelines", this->mPipelines));
	}
#pragma endregion

};

NS_END
