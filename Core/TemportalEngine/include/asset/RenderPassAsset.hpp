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

	struct NodeAttachment
	{
		math::Vector2 nodePosition;
		graphics::RenderPassAttachment data;

		bool operator!=(NodeAttachment const& other) const { return !((*this) == other); }
		bool operator==(NodeAttachment const& other) const
		{ return this->nodePosition == other.nodePosition && this->data == other.data; }

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("position", this->nodePosition));
			archive(cereal::make_nvp("data", this->data));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("position", this->nodePosition));
			archive(cereal::make_nvp("data", this->data));
		}
	};
	struct NodePhase
	{
		math::Vector2 nodePosition;
		graphics::RenderPassPhase data;

		bool operator!=(NodePhase const& other) const { return !((*this) == other); }
		bool operator==(NodePhase const& other) const
		{ return this->nodePosition == other.nodePosition && this->data == other.data; }

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("position", this->nodePosition));
			archive(cereal::make_nvp("data", this->data));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("position", this->nodePosition));
			archive(cereal::make_nvp("data", this->data));
		}
	};
	struct NodePhaseDependency
	{
		math::Vector2 nodePosition;
		bool bPrevPhaseIsRoot;
		graphics::RenderPassDependency data;

		bool operator!=(NodePhaseDependency const& other) const { return !((*this) == other); }
		bool operator==(NodePhaseDependency const& other) const
		{ return this->nodePosition == other.nodePosition && this->data == other.data && this->bPrevPhaseIsRoot == other.bPrevPhaseIsRoot; }

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("position", this->nodePosition));
			archive(cereal::make_nvp("prevPhaseIsRoot", this->bPrevPhaseIsRoot));
			archive(cereal::make_nvp("data", this->data));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("position", this->nodePosition));
			archive(cereal::make_nvp("prevPhaseIsRoot", this->bPrevPhaseIsRoot));
			archive(cereal::make_nvp("data", this->data));
		}
	};

	DECLARE_ASSET_CONTRUCTORS(RenderPass)
	DECLARE_PROPERTY_MUTATORS(std::optional<math::Color>, mClearColor, ClearColor)
	DECLARE_PROPERTY_MUTATORS(std::optional<DepthStencil>, mClearDepthStencil, ClearDepthStencil)
	DECLARE_PROPERTY_MUTATORS(graphics::Area, mRenderArea, RenderArea)
	DECLARE_PROPERTY_MUTATORS(std::vector<TypedAssetPath<Pipeline>>, mPipelines, PipelineRefs)
	DECLARE_PROPERTY_MUTATORS(std::vector<graphics::RPPhase>, mPhases, Phases)
	DECLARE_PROPERTY_MUTATORS(std::vector<graphics::RPDependency>, mPhaseDependencies, PhaseDependencies)
	DECLARE_PROPERTY_MUTATORS(std::vector<NodeAttachment>, mAttachmentNodes, AttachmentNodes)
	DECLARE_PROPERTY_MUTATORS(std::vector<NodePhase>, mPhaseNodes, PhaseNodes)
	DECLARE_PROPERTY_MUTATORS(std::vector<NodePhaseDependency>, mPhaseDependencyNodes, PhaseDependencyNodes)

	std::vector<AssetPath const*> getAssetRefs() const override;
	std::vector<AssetPath*> getAssetRefs() override;

private:

	std::optional<math::Color> mClearColor;
	std::optional<DepthStencil> mClearDepthStencil;
	graphics::Area mRenderArea;
	std::vector<TypedAssetPath<Pipeline>> mPipelines;

	std::vector<graphics::RPPhase> mPhases;
	std::vector<graphics::RPDependency> mPhaseDependencies;

	std::vector<NodeAttachment> mAttachmentNodes;
	std::vector<NodePhase> mPhaseNodes;
	std::vector<NodePhaseDependency> mPhaseDependencyNodes;

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

		SAVE_PROPERTY("phases", mPhases);
		SAVE_PROPERTY("phaseDependencies", mPhaseDependencies);

		SAVE_PROPERTY("attachments", mAttachmentNodes);
		SAVE_PROPERTY("phaseNodes", mPhaseNodes);
		SAVE_PROPERTY("phaseDependencyNodes", mPhaseDependencyNodes);
	}

	template <typename Archive>
	void deserialize(Archive &archive, bool bCheckDefaults)
	{
		Asset::deserialize(archive, bCheckDefaults);
		LOAD_PROPERTY("clearColor", mClearColor);
		LOAD_PROPERTY("clearDepthStencil", mClearDepthStencil);
		LOAD_PROPERTY("renderArea", mRenderArea);

		LOAD_PROPERTY("phases", mPhases);
		LOAD_PROPERTY("phaseDependencies", mPhaseDependencies);

		LOAD_PROPERTY("attachments", mAttachmentNodes);
		LOAD_PROPERTY("phaseNodes", mPhaseNodes);
		LOAD_PROPERTY("phaseDependencyNodes", mPhaseDependencyNodes);
	}
#pragma endregion

};

NS_END
