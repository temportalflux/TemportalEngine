#include "graphics/FontAtlas.hpp"

using namespace graphics;

Font::Font()
{
	this->mImage
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		//.setSize(math::Vector3UInt(face.getAtlasSize()).z(1))
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
}

Font::Font(Font &&other)
{
	*this = std::move(other);
}

Font& Font::operator=(Font &&other)
{
	this->mAtlasSize = other.mAtlasSize;
	this->mAtlasPixels = std::move(other.mAtlasPixels);
	this->mCharToGlyphIdx = std::move(other.mCharToGlyphIdx);
	this->mGlyphSprites = std::move(other.mGlyphSprites);
	this->mImage = std::move(other.mImage);
	this->mView = std::move(other.mView);
	this->mpSampler = other.mpSampler;
	this->mDescriptorSet = std::move(other.mDescriptorSet);
	return *this;
}

Font::~Font()
{
	this->mView.invalidate();
	this->mImage.invalidate();
}

void Font::setSampler(graphics::ImageSampler *sampler)
{
	this->mpSampler = sampler;
}

math::Vector2UInt& Font::atlasSize() { return this->mAtlasSize; }
std::vector<ui8>& Font::atlasPixels() { return this->mAtlasPixels; }
graphics::DescriptorSet const& Font::descriptorSet() const { return this->mDescriptorSet; }
graphics::DescriptorSet& Font::descriptorSet() { return this->mDescriptorSet; }

void Font::addGlyph(char code, GlyphSprite&& sprite)
{
	uIndex idx = this->mGlyphSprites.size();
	this->mGlyphSprites.push_back(sprite);
	this->mCharToGlyphIdx[code] = idx;
}

void Font::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mImage.setDevice(device);
	this->mView.setDevice(device);
}

void Font::initializeImage(graphics::CommandPool* transientPool)
{
	assert(this->mAtlasSize.x() > 0 && this->mAtlasSize.y() > 0);
	this->mImage.setSize(math::Vector3UInt(this->mAtlasSize).z(1)).create();
	this->mImage.transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, transientPool);
	this->mImage.writeImage((void*)this->mAtlasPixels.data(), this->mAtlasPixels.size() * sizeof(ui8), transientPool);
	this->mImage.transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, transientPool);
	this->mView.setImage(&this->mImage, vk::ImageAspectFlagBits::eColor).create();

	this->mAtlasPixels.clear();

	this->mDescriptorSet.attach("fontAtlas", graphics::EImageLayout::eShaderReadOnlyOptimal, &this->mView, this->mpSampler);
	this->mDescriptorSet.writeAttachments();
}

math::Vector<f32, 3> Font::measure(std::string const& str) const
{
	auto measurement = math::Vector<f32, 3>();
	for (auto const& c : str)
	{
		auto idxGlyph = this->mCharToGlyphIdx.find(c);
		assert(idxGlyph != this->mCharToGlyphIdx.end());
		auto const& glyph = this->mGlyphSprites[idxGlyph->second];
		measurement.x() += glyph.advance;
		measurement.y() = math::max(measurement.y(), glyph.size.y());
		measurement.z() = math::max(measurement.z(), glyph.bearing.y());
	}
	return measurement;
}

bool Font::contains(char const& code) const
{
	return this->mCharToGlyphIdx.find(code) != this->mCharToGlyphIdx.end();
}

Font::GlyphSprite const& Font::operator[](char const& code) const
{
	auto idxGlyph = this->mCharToGlyphIdx.find(code);
	assert(idxGlyph != this->mCharToGlyphIdx.end());
	return this->mGlyphSprites[idxGlyph->second];
}
