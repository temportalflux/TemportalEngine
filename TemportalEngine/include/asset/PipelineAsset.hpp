#pragma once

#include "asset/Asset.hpp"

#include "cereal/GraphicsFlags.hpp"
#include "cereal/list.hpp"
#include "cereal/mathVector.hpp"
#include "graphics/BlendMode.hpp"
#include "graphics/types.hpp"
#include "graphics/Area.hpp"

NS_ASSET
class Shader;

class Pipeline : public Asset
{
public:
	DECLARE_PROPERTY_CONTAINER(Pipeline)

	struct Descriptor
	{
		friend class cereal::access;

		std::string id;
		graphics::DescriptorType type;
		graphics::ShaderStage stage;

		bool operator==(Descriptor const& other) const { return id == other.id && type == other.type && stage == other.stage; }
		bool operator!=(Descriptor const& other) const { return !(*this == other); }

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("id", this->id));
			archive(cereal::make_nvp("type", this->type.value()));
			archive(cereal::make_nvp("stage", this->stage.value()));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("id", this->id));
			archive(cereal::make_nvp("type", this->type.value()));
			archive(cereal::make_nvp("stage", this->stage.value()));
		}
	};
	struct DescriptorGroup
	{
		std::vector<Descriptor> descriptors;

		bool operator==(DescriptorGroup const& other) const { return descriptors == other.descriptors; }
		bool operator!=(DescriptorGroup const& other) const { return !(*this == other); }

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("descriptors", this->descriptors));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("descriptors", this->descriptors));
		}
	};

public:
	DEFINE_ASSET_STATICS("pipeline", "Pipeline", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GRAPHICS);
	DECLARE_FACTORY_ASSET_METADATA()
	
	DECLARE_ASSET_CONTRUCTORS(Pipeline)
	DECLARE_PROPERTY_MUTATORS(TypedAssetPath<Shader>, mVertexShader, VertexShader)
	DECLARE_PROPERTY_MUTATORS(TypedAssetPath<Shader>, mFragmentShader, FragmentShader)
	DECLARE_PROPERTY_MUTATORS(graphics::Viewport, mViewport, Viewport)
	DECLARE_PROPERTY_MUTATORS(graphics::Area, mScissor, Scissor)
	DECLARE_PROPERTY_MUTATORS(graphics::FrontFace, mFrontFace, FrontFace)
	DECLARE_PROPERTY_MUTATORS(graphics::BlendMode, mBlendMode, BlendMode)
	DECLARE_PROPERTY_MUTATORS(graphics::PrimitiveTopology, mTopology, Topology)
	DECLARE_PROPERTY_MUTATORS(f32, mLineWidth, LineWidth)
	DECLARE_PROPERTY_MUTATORS(std::vector<DescriptorGroup>, mDescriptorGroups, DescriptorGroups)

	
private:

	TypedAssetPath<Shader> mVertexShader;
	TypedAssetPath<Shader> mFragmentShader;
	graphics::Viewport mViewport;
	graphics::Area mScissor;
	graphics::FrontFace mFrontFace;
	graphics::BlendMode mBlendMode;
	graphics::PrimitiveTopology mTopology;
	f32 mLineWidth;
	std::vector<DescriptorGroup> mDescriptorGroups;

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
		SAVE_PROPERTY("shader-vert", mVertexShader);
		SAVE_PROPERTY("shader-frag", mFragmentShader);
		SAVE_PROPERTY("viewport", mViewport);
		SAVE_PROPERTY("scissor", mScissor);
		SAVE_PROPERTY("frontFace", mFrontFace);
		SAVE_PROPERTY("blendMode", mBlendMode);
		SAVE_PROPERTY("topology", mTopology);
		SAVE_PROPERTY("lineWidth", mLineWidth);
		SAVE_PROPERTY("descriptorGroups", mDescriptorGroups);
	}

	template <typename Archive>
	void deserialize(Archive &archive, bool bCheckDefaults)
	{
		Asset::deserialize(archive, bCheckDefaults);
		LOAD_PROPERTY("shader-vert", mVertexShader);
		LOAD_PROPERTY("shader-frag", mFragmentShader);
		LOAD_PROPERTY("viewport", mViewport);
		LOAD_PROPERTY("scissor", mScissor);
		LOAD_PROPERTY("frontFace", mFrontFace);
		LOAD_PROPERTY("blendMode", mBlendMode);
		LOAD_PROPERTY("topology", mTopology);
		LOAD_PROPERTY("lineWidth", mLineWidth);
		LOAD_PROPERTY("descriptorGroups", mDescriptorGroups);
	}
#pragma endregion

};

NS_END
