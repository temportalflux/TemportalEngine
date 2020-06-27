#pragma once

#include "asset/Asset.hpp"

NS_ASSET

class TextureSampler : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_STATICS("sampler", "Texture Sampler", DEFAULT_ASSET_EXTENSION);
	DECLARE_FACTORY_ASSET_METADATA()

	TextureSampler() = default;
	CREATE_NEWASSET_CONSTRUCTOR(TextureSampler) {}

private:

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
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		Asset::deserialize(archive);
	}
#pragma endregion

};

NS_END
