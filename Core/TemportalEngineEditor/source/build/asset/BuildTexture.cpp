#include "build/asset/BuildTexture.hpp"

#include "asset/Asset.hpp"
#include "asset/Texture.hpp"
#include "utility/ImageUtils.hpp"

using namespace build;

std::shared_ptr<BuildAsset> BuildTexture::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildTexture>(asset);
}

BuildTexture::BuildTexture(std::shared_ptr<asset::Asset> asset) : BuildAsset(asset)
{
}

void BuildTexture::loadImage(std::filesystem::path const &path, math::Vector2UInt &sizeOut, std::vector<ui8> &pixels)
{
	static i32 LOAD_MODE = STBI_rgb_alpha;
	static ui32 LOAD_MODE_SIZE = 4; // 4 bytes per pixel
	i32 width, height, srcChannels;

	ui8* data = stbi_load(path.string().c_str(), &width, &height, &srcChannels, LOAD_MODE);
	if (data == nullptr) { return; }
	sizeOut = { (ui32)width, (ui32)height };

	pixels.resize(sizeOut.x() * sizeOut.y() * LOAD_MODE_SIZE);
	memcpy(pixels.data(), data, pixels.size());
	stbi_image_free(data);
}

std::vector<std::string> BuildTexture::compile(logging::Logger &logger)
{
	loadImage(this->get<asset::Texture>()->getAbsoluteSourcePath(), this->mSourceSize, this->mSourceBinary);
	return {};
}

void BuildTexture::save()
{
	this->get<asset::Texture>()->setSourceBinary(this->mSourceBinary, this->mSourceSize);
	BuildAsset::save();
}
