#pragma once

#include "asset/Asset.hpp"

#include "graphics/Area.hpp"
#include "graphics/RenderPassMeta.hpp"
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

	std::optional<math::Vector4> const& getClearColorValue() const { return this->mClearColor; }
	RenderPass& setClearColor(std::optional<math::Vector4>& color) { this->mClearColor = color; return *this; }
	std::optional<std::pair<f32, ui32>> const& getDepthStencilClearValues() const { return this->mClearDepthStencil; }
	RenderPass& setDepthStencilClearValues(std::optional<std::pair<f32, ui32>> const& values) { this->mClearDepthStencil = values; return *this; }
	graphics::Area const& getRenderArea() const { return this->mRenderArea; }
	RenderPass& setRenderArea(graphics::Area const& area) { this->mRenderArea = area; return *this; }
	std::vector<TypedAssetPath<Pipeline>> const& getPipelineRefs() const { return this->mPipelines; }
	RenderPass& setPipelineRefs(std::vector<TypedAssetPath<Pipeline>> const& refs) { this->mPipelines = refs; return *this; }
	std::vector<graphics::RPPhase> const& getPhases() const { return this->mPhases; }
	RenderPass& setPhases(std::vector<graphics::RPPhase> const& phases) { this->mPhases = phases; return *this; }
	std::vector<graphics::RPDependency> const& getPhaseDependencies() const { return this->mPhaseDependencies; }
	RenderPass& setPhaseDependencies(std::vector<graphics::RPDependency> const& deps) { this->mPhaseDependencies = deps; return *this; }

private:

	std::optional<math::Vector4> mClearColor;
	std::optional<std::pair<f32, ui32>> mClearDepthStencil;
	graphics::Area mRenderArea;
	std::vector<TypedAssetPath<Pipeline>> mPipelines;

	std::vector<graphics::RPPhase> mPhases;
	std::vector<graphics::RPDependency> mPhaseDependencies;

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
		archive(cereal::make_nvp("phases", this->mPhases));
		archive(cereal::make_nvp("phaseDependencies", this->mPhaseDependencies));
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		Asset::deserialize(archive);
		archive(cereal::make_nvp("clearColor", this->mClearColor));
		archive(cereal::make_nvp("clearDepthStencil", this->mClearDepthStencil));
		archive(cereal::make_nvp("renderArea", this->mRenderArea));
		archive(cereal::make_nvp("pipelines", this->mPipelines));
		archive(cereal::make_nvp("phases", this->mPhases));
		archive(cereal::make_nvp("phaseDependencies", this->mPhaseDependencies));
	}
#pragma endregion

};

NS_END
