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
	friend class cereal::access;

public:
	struct Descriptor
	{
		friend class cereal::access;

		std::string id;
		graphics::DescriptorType::Enum type;
		graphics::ShaderStage::Enum stage;

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("id", this->id));
			archive(cereal::make_nvp("type", this->type));
			archive(cereal::make_nvp("stage", this->stage));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("id", this->id));
			archive(cereal::make_nvp("type", this->type));
			archive(cereal::make_nvp("stage", this->stage));
		}
	};
	struct DescriptorGroup
	{
		std::vector<Descriptor> descriptors;

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
	
	Pipeline() = default;
	CREATE_NEWASSET_CONSTRUCTOR(Pipeline) {}

	TypedAssetPath<Shader> getVertexShader() const { return this->mVertexShader; }
	Pipeline& setVertexShader(TypedAssetPath<Shader> const& value) { this->mVertexShader = value; return *this; }
	TypedAssetPath<Shader> getFragmentShader() const { return this->mFragmentShader; }
	Pipeline& setFragmentShader(TypedAssetPath<Shader> const& value) { this->mFragmentShader = value; return *this; }

	graphics::Viewport const& getViewport() const { return this->mViewport; }
	Pipeline& setViewport(graphics::Viewport const& value) { this->mViewport = value; return *this; }
	graphics::Area const& getScissor() const { return this->mScissor; }
	Pipeline& setScissor(graphics::Area const& value) { this->mScissor = value; return *this; }
	graphics::FrontFace::Enum const& getFrontFace() const { return this->mFrontFace; }
	Pipeline& setFrontFace(graphics::FrontFace::Enum const& value) { this->mFrontFace = value; return *this; }
	graphics::BlendMode const& getBlendMode() const { return this->mBlendMode; }
	Pipeline& setBlendMode(graphics::BlendMode const& value) { this->mBlendMode = value; return *this; }
	std::vector<DescriptorGroup> const& getDescriptorGroups() const { return this->mDescriptorGroups; }
	Pipeline& setDescriptorGroups(std::vector<DescriptorGroup> const& value) { this->mDescriptorGroups = value; return *this; }

private:

	TypedAssetPath<Shader> mVertexShader;
	TypedAssetPath<Shader> mFragmentShader;

	graphics::Viewport mViewport;
	graphics::Area mScissor;
	graphics::FrontFace::Enum mFrontFace;
	graphics::BlendMode mBlendMode;
	std::vector<DescriptorGroup> mDescriptorGroups;

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
		archive(cereal::make_nvp("shader-vert", this->mVertexShader));
		archive(cereal::make_nvp("shader-frag", this->mFragmentShader));
		archive(cereal::make_nvp("viewport", this->mViewport));
		archive(cereal::make_nvp("scissor", this->mScissor));
		archive(cereal::make_nvp("frontFace", this->mFrontFace));
		archive(cereal::make_nvp("blendMode", this->mBlendMode));
		archive(cereal::make_nvp("descriptorGroups", this->mDescriptorGroups));
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		Asset::deserialize(archive);
		archive(cereal::make_nvp("shader-vert", this->mVertexShader));
		archive(cereal::make_nvp("shader-frag", this->mFragmentShader));
		archive(cereal::make_nvp("viewport", this->mViewport));
		archive(cereal::make_nvp("scissor", this->mScissor));
		archive(cereal::make_nvp("frontFace", this->mFrontFace));
		archive(cereal::make_nvp("blendMode", this->mBlendMode));
		archive(cereal::make_nvp("descriptorGroups", this->mDescriptorGroups));
	}
#pragma endregion

};

NS_END
