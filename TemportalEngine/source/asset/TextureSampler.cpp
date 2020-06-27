#include "asset/TextureSampler.hpp"

#include "asset/AssetManager.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(TextureSampler)

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, TextureSampler::write, cereal::JSONOutputArchive, TextureSampler::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, TextureSampler::read, cereal::JSONInputArchive, TextureSampler::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, TextureSampler::compile, cereal::PortableBinaryOutputArchive, TextureSampler::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, TextureSampler::decompile, cereal::PortableBinaryInputArchive, TextureSampler::deserialize);
