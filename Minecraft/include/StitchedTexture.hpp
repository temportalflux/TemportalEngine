#pragma once

#include "CoreInclude.hpp"

#include "asset/AssetPath.hpp"

FORWARD_DEF(NS_ASSET, class Texture);
FORWARD_DEF(NS_ASSET, class TextureSampler);
FORWARD_DEF(NS_GRAPHICS, class GameRenderer);
FORWARD_DEF(NS_GRAPHICS, class Image);
FORWARD_DEF(NS_GRAPHICS, class ImageSampler);
FORWARD_DEF(NS_GRAPHICS, class ImageView);
FORWARD_DEF(NS_GRAPHICS, class Memory);

class StitchedTexture
{

public:

	struct Entry
	{
		math::Vector2 offset;
		math::Vector2 size;
	};

	StitchedTexture() = default;
	StitchedTexture(math::Vector2UInt minSize, math::Vector2UInt maxSize, math::Vector2UInt unitSize);

	math::Vector2UInt getSizePerEntry() const { return this->mSizePerEntry; }
	bool canAdd(uSize const count) const;
	bool addTextures(std::vector<std::pair<asset::AssetPath, std::shared_ptr<asset::Texture>>> textures);
	void finalize(std::shared_ptr<graphics::GameRenderer> renderer);
	std::optional<Entry> getStitchedTexture(asset::AssetPath const &path) const;

private:
	struct InternalEntry
	{
		math::Vector2UInt offset;
	};

	// Config Vars
	math::Vector2UInt mSizeMin, mSizeMax, mSize;
	math::Vector2UInt mSizePerEntry;
	
	// Entry Management
	std::vector<InternalEntry> mEntries;
	std::unordered_map<asset::AssetPath, uIndex> mEntryByPath;

	// Builder
	std::vector<ui8> mPixelData;
	std::optional<math::Vector2UInt> mLastOffsetAllocated;

	// Post-Stitch
	bool mbHasWrittenStitchings;
	std::shared_ptr<graphics::Memory> mpImageMemory;
	std::shared_ptr<graphics::Image> mpImage;
	std::shared_ptr<graphics::ImageView> mpView;

	void initializePixelData();
	void writePixelData(math::Vector2UInt const &offset, std::vector<ui8> const &data);
	bool canFitAdditionalTextures(uSize count) const;
	std::optional<math::Vector2UInt> findNextOffset(math::Vector2UInt const &prevAllocatedOffset) const;
	math::Vector2UInt getNextSize() const;
	bool canIncreaseSize() const;
	void increaseSize();

};
