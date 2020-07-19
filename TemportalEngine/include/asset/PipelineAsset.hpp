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
	DEFINE_ASSET_STATICS("pipeline", "Pipeline", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GRAPHICS);
	DECLARE_FACTORY_ASSET_METADATA()
	
	Pipeline() = default;
	CREATE_NEWASSET_CONSTRUCTOR(Pipeline) {}

public:
	struct Descriptor
	{
		friend class cereal::access;

		std::string id;
		vk::DescriptorType type;
		vk::ShaderStageFlagBits stage;

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

private:

	TypedAssetPath<Shader> mVertexShader;
	TypedAssetPath<Shader> mFragmentShader;

	graphics::Viewport mViewport;
	graphics::Area mScissor;
	graphics::FrontFace::Enum mFrontFace;
	graphics::BlendMode mBlendMode;
	std::vector<Descriptor> mDescriptors;

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
		archive(cereal::make_nvp("descriptors", this->mDescriptors));
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
		archive(cereal::make_nvp("descriptors", this->mDescriptors));
	}
#pragma endregion

};

NS_END
