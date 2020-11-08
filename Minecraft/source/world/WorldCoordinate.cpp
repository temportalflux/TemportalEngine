#include "world/WorldCoordinate.hpp"

using namespace world;

Coordinate::Coordinate(math::Vector3Int chunk, math::Vector3Int local)
	: mChunkPosition(chunk), mBlockPosition(local)
{
}

bool Coordinate::operator==(Coordinate const& other) const
{
	return
		mChunkPosition == other.mChunkPosition
		&& mBlockPosition == other.mBlockPosition
		&& mBlockOffset == other.mBlockOffset;
}

bool Coordinate::operator!=(Coordinate const& other) const
{
	return !(*this == other);
}

void Coordinate::operator=(Coordinate const &other)
{
	this->mChunkPosition = other.mChunkPosition;
	this->mBlockPosition = other.mBlockPosition;
	this->mBlockOffset = other.mBlockOffset;
}

void Coordinate::operator+=(Coordinate const &other)
{
	this->mChunkPosition += other.mChunkPosition;
	this->mBlockPosition += other.mBlockPosition;
	this->mBlockOffset += other.mBlockOffset;
	this->adjustForChunkSize();
}

Coordinate const Coordinate::operator+(Coordinate const &other) const
{
	Coordinate result = *this;
	result += other;
	return result;
}

void Coordinate::operator-=(Coordinate const &other)
{
	this->mChunkPosition -= other.mChunkPosition;
	this->mBlockPosition -= other.mBlockPosition;
	this->mBlockOffset -= other.mBlockOffset;
	this->adjustForChunkSize();
}

Coordinate const Coordinate::operator-(Coordinate const &other) const
{
	Coordinate result = *this;
	result -= other;
	return result;
}

Coordinate const Coordinate::operator+(math::Vector3Int const &other) const
{
	return *this + Coordinate(this->mChunkPosition, this->mBlockPosition + other);
}

Coordinate const Coordinate::operator-(math::Vector3Int const &other) const
{
	return *this - Coordinate(this->mChunkPosition, this->mBlockPosition + other);
}

bool Coordinate::operator<(Coordinate const& other) const
{
	// Sort by chunk position, then by block position, ignoring the offset
	// Each vector will be sorted by `z` first, then `y`, then `x`

	// Compare the chunk
	if (this->mChunkPosition.z() != other.mChunkPosition.z())
		return this->mChunkPosition.z() < other.mChunkPosition.z();
	if (this->mChunkPosition.y() != other.mChunkPosition.y())
		return this->mChunkPosition.y() < other.mChunkPosition.y();
	if (this->mChunkPosition.x() != other.mChunkPosition.x())
		return this->mChunkPosition.x() < other.mChunkPosition.x();

	// Compare the block pos
	if (this->mBlockPosition.z() != other.mBlockPosition.z())
		return this->mBlockPosition.z() < other.mBlockPosition.z();
	if (this->mBlockPosition.y() != other.mBlockPosition.y())
		return this->mBlockPosition.y() < other.mBlockPosition.y();
	if (this->mBlockPosition.x() != other.mBlockPosition.x())
		return this->mBlockPosition.x() < other.mBlockPosition.x();
	
	// If both chunk and block pos are equivalent, then the coordinates are considered equivalent (even if there is offset)
	return false;
}

void Coordinate::adjustForChunkSize()
{
	this->mBlockPosition += this->mBlockOffset.removeExcess(1);
	this->mChunkPosition += this->mBlockPosition.removeExcess((i32)ChunkSize());
}
