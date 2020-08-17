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

void Coordinate::adjustForChunkSize()
{
	this->mBlockPosition += this->mBlockOffset.removeExcess(1);
	this->mChunkPosition += this->mBlockPosition.removeExcess((i32)ChunkSize());
}
