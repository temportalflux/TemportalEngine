#include "build/asset/BuildTexture.hpp"

#include "asset/Asset.hpp"
#include "asset/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace build;

std::shared_ptr<BuildAsset> BuildTexture::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildTexture>(asset);
}

BuildTexture::BuildTexture(std::shared_ptr<asset::Asset> asset) : BuildAsset(asset)
{
}

std::vector<ui8> BuildTexture::loadImage(std::filesystem::path const &path, math::Vector2UInt &sizeOut)
{
	static i32 LOAD_MODE = STBI_rgb_alpha;
	static ui32 LOAD_MODE_SIZE = 4; // 4 bytes per pixel
	i32 width, height, srcChannels;

	ui8* data = stbi_load(path.string().c_str(), &width, &height, &srcChannels, LOAD_MODE);
	if (data == nullptr)
	{
		return {};
	}
	sizeOut = { (ui32)width, (ui32)height };

	std::vector<ui8> imageData(sizeOut.x() * sizeOut.y() * LOAD_MODE_SIZE);
	memcpy(imageData.data(), data, imageData.capacity());
	stbi_image_free(data);

	return imageData;
}

std::vector<std::string> BuildTexture::compile(logging::Logger &logger)
{
	this->mSourceBinary = loadImage(this->get<asset::Texture>()->getAbsoluteSourcePath(), this->mSourceSize);
	return {};
}

void BuildTexture::save()
{
	this->get<asset::Texture>()->setSourceBinary(this->mSourceBinary, this->mSourceSize);
	BuildAsset::save();
}
