#pragma once

#include "asset/Asset.hpp"

NS_ASSET

class BlockType : public Asset
{
	friend class cereal::access;
	typedef TypedAssetPath<class Texture> TextureAssetPath;

public:
	DEFINE_ASSET_STATICS("block", "BlockType", DEFAULT_ASSET_EXTENSION);
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
			this->right.write("right", archive);
			this->left.write("left", archive);
			this->front.write("front", archive);
			this->back.write("back", archive);
			this->up.write("up", archive);
			this->down.write("down", archive);
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			this->right.read("right", archive);
			this->left.read("left", archive);
			this->front.read("front", archive);
			this->back.read("back", archive);
			this->up.read("up", archive);
			this->down.read("down", archive);
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
