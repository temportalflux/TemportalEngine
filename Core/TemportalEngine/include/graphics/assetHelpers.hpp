#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/TypedAssetPath.hpp"

FORWARD_DEF(NS_ASSET, class Texture);
FORWARD_DEF(NS_ASSET, class TextureSampler);

NS_GRAPHICS
class Image;
class ImageSampler;

std::vector<ui8> populateImage(graphics::Image* image, asset::TypedAssetPath<asset::Texture> const& path);
void populateSampler(graphics::ImageSampler* sampler, asset::TypedAssetPath<asset::TextureSampler> const& path);

NS_END
