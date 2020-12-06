#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/TypedAssetPath.hpp"

FORWARD_DEF(NS_ASSET, class Texture);
FORWARD_DEF(NS_ASSET, class TextureSampler);
FORWARD_DEF(NS_ASSET, class Pipeline);

NS_GRAPHICS
class Image;
class ImageSampler;
class Pipeline;
class DescriptorLayout;

std::vector<ui8> populateImage(graphics::Image* image, asset::TypedAssetPath<asset::Texture> const& path);
void populateSampler(graphics::ImageSampler* sampler, asset::TypedAssetPath<asset::TextureSampler> const& path);

void populatePipeline(
	asset::TypedAssetPath<asset::Pipeline> const& path,
	graphics::Pipeline* pipeline, graphics::DescriptorLayout* layout
);

NS_END
