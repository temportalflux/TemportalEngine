#include "asset/MinecraftAssetStatics.hpp"

#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"

asset::AssetPath asset::SAMPLER_NEAREST_NEIGHBOR = asset::AssetPath(
	asset::TextureSampler::StaticType(), "assets/textures/NearestNeighborSampler.te-asset"
);
