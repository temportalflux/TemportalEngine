#pragma once

#include "asset/Asset.hpp"

NS_ASSET

class BlockType : public Asset
{
	friend class cereal::access;
	typedef TypedAssetPath<class Texture> TextureAssetPath;

public:
	DEFINE_ASSET_STATICS("block", "BlockType", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GENERAL);
	DECLARE_FACTORY_ASSET_METADATA()

public:
	BlockType() = default;
	BlockType(std::filesystem::path filePath);

	std::string const uniqueId() const;

private:

	struct BlockIdentifier
	{
		friend class cereal::access;

		// TODO: Replace with an asset referenced
		std::string moduleName;

		/**
		 * The unique name of the block type.
		 */
		std::string name;

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("module", this->moduleName));
			archive(cereal::make_nvp("name", this->name));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("module", this->moduleName));
			archive(cereal::make_nvp("name", this->name));
		}

	};
	
	BlockIdentifier mId;

	struct BlockTextureSet
	{
		friend class cereal::access;

		TextureAssetPath right;
		TextureAssetPath left;
		TextureAssetPath front;
		TextureAssetPath back;
		TextureAssetPath up;
		TextureAssetPath down;

		template <typename Archive>
		void save(Archive &archive) const
		{
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
			archive(cereal::make_nvp("right", this->right));
			archive(cereal::make_nvp("left", this->left));
			archive(cereal::make_nvp("front", this->front));
			archive(cereal::make_nvp("back", this->back));
			archive(cereal::make_nvp("up", this->up));
			archive(cereal::make_nvp("down", this->down));
		}
	};

	BlockTextureSet mTextures;

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
