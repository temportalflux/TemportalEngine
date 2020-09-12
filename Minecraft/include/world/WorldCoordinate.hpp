#pragma once

#include "CoreInclude.hpp"

NS_WORLD

constexpr uSize ChunkSize() { return 16; }

class Coordinate
{

public:
	Coordinate() = default;
	Coordinate(math::Vector3Int chunk, math::Vector3Int local);

	math::Vector3Int chunk() const { return this->mChunkPosition; }
	math::Vector3Int local() const { return this->mBlockPosition; }

	bool operator==(Coordinate const& other) const;
	bool operator!=(Coordinate const& other) const;
	void operator=(Coordinate const &other);
	void operator+=(Coordinate const &other);
	Coordinate const operator+(Coordinate const &other) const;
	void operator-=(Coordinate const &other);
	Coordinate const operator-(Coordinate const &other) const;
	Coordinate const operator+(math::Vector3Int const &other) const;
	Coordinate const operator-(math::Vector3Int const &other) const;

private:
	math::Vector3Int mChunkPosition;
	math::Vector3Int mBlockPosition;
	math::Vector3 mBlockOffset;

	void adjustForChunkSize();

};

NS_END
