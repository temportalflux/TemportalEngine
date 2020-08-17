#pragma once

#include "asset/Asset.hpp"
#include "BlockId.hpp"

NS_ASSET
class TextureSampler;

class BlockType : public Asset
{
	friend class cereal::access;
	typedef TypedAssetPath<class Texture> TextureAssetPath;

public:
	DEFINE_ASSET_STATICS("block", "BlockType", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GENERAL);
	DECLARE_FACTORY_ASSET_METADATA()

	struct TextureSet
	{
		friend class cereal::access;

		TypedAssetPath<asset::TextureSampler> sampler;
		TextureAssetPath right;
		TextureAssetPath left;
		TextureAssetPath front;
		TextureAssetPath back;
		TextureAssetPath up;
		TextureAssetPath down;

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("sampler", this->sampler));
			archive(cereal::make_nvp("right", this->right));
			archive(cereal::make_nvp("left", this->left));
			archive(cereal::make_nvp("front", this->front));
			archive(cereal::make_nvp("back", this->back));
			archive(cereal::make_nvp("up", this->up));
			archive(cereal::make_nvp("down", this->down));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("sampler", this->sampler));
			archive(cereal::make_nvp("right", this->right));
			archive(cereal::make_nvp("left", this->left));
			archive(cereal::make_nvp("front", this->front));
			archive(cereal::make_nvp("back", this->back));
			archive(cereal::make_nvp("up", this->up));
			archive(cereal::make_nvp("down", this->down));
		}
	};

public:
	BlockType() = default;
	BlockType(std::filesystem::path filePath);

	game::BlockId const& uniqueId() const;
	TextureSet const& textureSet() const { return this->mTextures; }

private:
	
	game::BlockId mId;

	TextureSet mTextures;

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
		archive(cereal::make_nvp("id", this->mId));
		archive(cereal::make_nvp("textures", this->mTextures));
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		Asset::deserialize(archive);
		archive(cereal::make_nvp("id", this->mId));
		archive(cereal::make_nvp("textures", this->mTextures));
	}
#pragma endregion

};

NS_END
