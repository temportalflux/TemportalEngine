#include "StitchedTexture.hpp"

#include "asset/Texture.hpp"
#include "graphics/GameRenderer.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/Memory.hpp"

ui32 CHANNELS_PER_PIXEL = 4;
vk::Format FORMAT = vk::Format::eR8G8B8A8Srgb;

StitchedTexture::StitchedTexture(math::Vector2UInt minSize, math::Vector2UInt maxSize, math::Vector2UInt unitSize)
	: mSizeMin(minSize), mSizeMax(maxSize), mSize(minSize)
	, mSizePerEntry(unitSize)
	, mbHasWrittenStitchings(false)
{
	assert(minSize.x() > unitSize.x() && minSize.y() > unitSize.y());
	this->initializePixelData();
}

void StitchedTexture::initializePixelData()
{
	this->mPixelData = std::vector<ui8>(this->mSize.x() * this->mSize.y() * CHANNELS_PER_PIXEL);
}

bool StitchedTexture::canAdd(uSize const count) const
{
	return this->canFitAdditionalTextures(count) || this->canIncreaseSize();
}

bool StitchedTexture::addTextures(std::vector<std::pair<asset::AssetPath, std::shared_ptr<asset::Texture>>> textures)
{
	if (this->mbHasWrittenStitchings) return false;

	// Filter out any entries which already exist
	textures.erase(std::remove_if(
		textures.begin(), textures.end(),
		[&](auto const &texture)
		{
			return this->mEntryByPath.find(texture.first) != this->mEntryByPath.end();
		}
	), textures.end());
	for (auto texture : textures)
	{
		// Confirm that all new textures have the appropriate size
		if (texture.second->getSourceSize() != this->mSizePerEntry) return false;
	}

	uSize count = textures.size();
	if (!this->canFitAdditionalTextures(count))
	{
		if (!this->canIncreaseSize()) return false;
		this->increaseSize();
		if (!this->canFitAdditionalTextures(count)) return false;
	}

	std::optional<math::Vector2UInt> nextOffset;
	for (uIndex i = 0; i < count; ++i)
	{
		nextOffset = this->mLastOffsetAllocated ? this->findNextOffset(*this->mLastOffsetAllocated) : math::Vector2UInt({ 0, 0 });
		if (!nextOffset) return false;
		this->mLastOffsetAllocated = *nextOffset;

		auto assetPath = textures[i].first;
		auto iter = this->mEntries.insert(this->mEntries.end(), { *nextOffset });
		this->mEntryByPath.insert(std::make_pair(assetPath, std::distance(this->mEntries.begin(), iter)));
		this->writePixelData(*nextOffset, textures[i].second->getSourceBinary());
	}

	return true;
}

void StitchedTexture::writePixelData(math::Vector2UInt const &offset, std::vector<ui8> const &src)
{
	assert(src.size() == this->mSizePerEntry.x() * this->mSizePerEntry.y() * CHANNELS_PER_PIXEL);
	math::Vector2UInt srcPos;
	for (srcPos.x() = 0; srcPos.x() < this->mSizePerEntry.x(); ++srcPos.x())
	{
		for (srcPos.y() = 0; srcPos.y() < this->mSizePerEntry.y(); ++srcPos.y())
		{
			auto dstPos = offset + srcPos;
			uIndex srcIdx = (srcPos.y() * this->mSizePerEntry.y() + srcPos.x()) * CHANNELS_PER_PIXEL;
			uIndex dstIdx = (dstPos.y() * this->mSizePerEntry.y() + dstPos.x()) * CHANNELS_PER_PIXEL;
			for (uIndex channel = 0; channel < CHANNELS_PER_PIXEL; ++channel)
			{
				this->mPixelData[dstIdx + channel] = src[srcIdx + channel];
			}
		}
	}
}

bool StitchedTexture::canFitAdditionalTextures(uSize count) const
{
	math::Vector2UInt prevOffset = this->mLastOffsetAllocated ? *this->mLastOffsetAllocated : math::Vector2UInt({ 0, 0 });
	std::optional<math::Vector2UInt> nextOffset;
	for (uIndex i = 0; i < count; ++i)
	{
		nextOffset = this->findNextOffset(prevOffset);
		if (!nextOffset) return false;
		prevOffset = *nextOffset;
	}
	return true;
}

std::optional<math::Vector2UInt> StitchedTexture::findNextOffset(math::Vector2UInt const &prevAllocatedOffset) const
{
	math::Vector2UInt nextOffset = prevAllocatedOffset;

	// shift to the next slot in the row
	nextOffset.x() += this->mSizePerEntry.x();
	// if the next slot is within the size of the row, its valid
	if (nextOffset.x() + this->mSizePerEntry.x() <= this->mSize.x()) return nextOffset;
	
	// shifted out of row, lets move the slot to the start of the next row
	nextOffset.x() = 0;
	nextOffset.y() += this->mSizePerEntry.y();
	// if this new row is within height bounds, then its valid
	if (nextOffset.y() + this->mSizePerEntry.y() <= this->mSize.y()) return nextOffset;

	// out of space in texture. will need to scale up
	return std::nullopt;
}

math::Vector2UInt StitchedTexture::getNextSize() const
{
	return { this->mSize.x() * this->mSize.x(), this->mSize.y() * this->mSize.y() };
}

bool StitchedTexture::canIncreaseSize() const
{
	math::Vector2UInt nextSize = this->getNextSize();
	return nextSize.x() <= this->mSizeMax.x() && nextSize.y() <= this->mSizeMax.y();
}

void StitchedTexture::increaseSize()
{
	this->mSize = this->getNextSize();
	this->initializePixelData();
	// Rebuild offsets
	this->mLastOffsetAllocated = math::Vector2UInt({ 0, 0 });
	for (auto& entry : this->mEntries)
	{
		auto nextOffset = this->mLastOffsetAllocated ? this->findNextOffset(*this->mLastOffsetAllocated) : math::Vector2UInt({ 0, 0 });
		assert((bool)nextOffset);
		entry.offset = *nextOffset;
		this->mLastOffsetAllocated = *nextOffset;
	}
}

void StitchedTexture::finalize(std::shared_ptr<graphics::GameRenderer> renderer)
{
	auto graphicsDevice = renderer->getDevice();
	auto& cmdPool = renderer->getTransientPool();

	this->mpImageMemory = std::make_shared<graphics::Memory>();
	this->mpImageMemory->setDevice(graphicsDevice);
	this->mpImageMemory->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->mpImage = std::make_shared<graphics::Image>();
	this->mpImage->setDevice(graphicsDevice);
	this->mpImage
		->setFormat(FORMAT)
		.setSize(math::Vector3UInt(this->mSize).z(1))
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	this->mpImage->create();
	this->mpImage->configureSlot(this->mpImageMemory);

	this->mpImageMemory->create();

	this->mpImage->bindMemory();
	this->mpImage->transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, &cmdPool);
	this->mpImage->writeImage((void*)this->mPixelData.data(), this->mPixelData.size() * sizeof(ui8), &cmdPool);
	this->mpImage->transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, &cmdPool);

	this->mpView = std::make_shared<graphics::ImageView>();
	this->mpView->setDevice(graphicsDevice);
	this->mpView->setImage(this->mpImage.get(), vk::ImageAspectFlagBits::eColor);
	this->mpView->create();

	this->mbHasWrittenStitchings = true;
	this->mPixelData.clear();
}

std::optional<StitchedTexture::Entry> StitchedTexture::getStitchedTexture(asset::AssetPath const &path) const
{
	if (!this->mbHasWrittenStitchings) return std::nullopt;
	auto iter = this->mEntryByPath.find(path);
	if (iter == this->mEntryByPath.end()) return std::nullopt;
	math::Vector2 currentSize = this->mSize.toFloat();
	Entry entry = {
		this->mEntries[iter->second].offset.toFloat() / currentSize,
		this->mSizePerEntry.toFloat() / currentSize
	};
	return entry;
}
