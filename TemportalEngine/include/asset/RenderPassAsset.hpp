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
public:
	typedef std::pair<f32, ui32> DepthStencil;
	DECLARE_PROPERTY_CONTAINER(RenderPass)
	DEFINE_ASSET_STATICS("renderpass", "Render Pass", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GRAPHICS);
	DECLARE_FACTORY_ASSET_METADATA()

	DECLARE_ASSET_CONTRUCTORS(RenderPass)
	DECLARE_PROPERTY_MUTATORS(std::optional<math::Color>, mClearColor, ClearColor)
	DECLARE_PROPERTY_MUTATORS(std::optional<DepthStencil>, mClearDepthStencil, ClearDepthStencil)
	DECLARE_PROPERTY_MUTATORS(graphics::Area, mRenderArea, RenderArea)
	DECLARE_PROPERTY_MUTATORS(std::vector<TypedAssetPath<Pipeline>>, mPipelines, PipelineRefs)
	DECLARE_PROPERTY_MUTATORS(std::vector<graphics::RPPhase>, mPhases, Phases)
	DECLARE_PROPERTY_MUTATORS(std::vector<graphics::RPDependency>, mPhaseDependencies, PhaseDependencies)

private:

	std::optional<math::Color> mClearColor;
	std::optional<DepthStencil> mClearDepthStencil;
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
	void serialize(Archive &archive, bool bCheckDefaults) const
	{
		Asset::serialize(archive, bCheckDefaults);
		SAVE_PROPERTY("clearColor", mClearColor);
		SAVE_PROPERTY("clearDepthStencil", mClearDepthStencil);
		SAVE_PROPERTY("renderArea", mRenderArea);
		SAVE_PROPERTY("pipelines", mPipelines);
		SAVE_PROPERTY("phases", mPhases);
		SAVE_PROPERTY("phaseDependencies", mPhaseDependencies);
	}

	template <typename Archive>
	void deserialize(Archive &archive, bool bCheckDefaults)
	{
		Asset::deserialize(archive, bCheckDefaults);
		LOAD_PROPERTY("clearColor", mClearColor);
		LOAD_PROPERTY("clearDepthStencil", mClearDepthStencil);
		LOAD_PROPERTY("renderArea", mRenderArea);
		LOAD_PROPERTY("pipelines", mPipelines);
		LOAD_PROPERTY("phases", mPhases);
		LOAD_PROPERTY("phaseDependencies", mPhaseDependencies);
	}
#pragma endregion

};

NS_END
