#include "asset/PipelineAsset.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(Pipeline)


CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Pipeline::write, cereal::JSONOutputArchive, Pipeline::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Pipeline::read, cereal::JSONInputArchive, Pipeline::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Pipeline::compile, cereal::PortableBinaryOutputArchive, Pipeline::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Pipeline::decompile, cereal::PortableBinaryInputArchive, Pipeline::deserialize);
