#pragma once

#include "CoreInclude.hpp"

NS_WORLD

constexpr uSize ChunkSize() { return CHUNK_SIDE_LENGTH; }

class Coordinate
{

public:
	Coordinate() = default;
	Coordinate(math::Vector3Int chunk, math::Vector3Int local);
	Coordinate(math::Vector3Int const& chunk, math::Vector3Int const& local, math::Vector3 const& offset);

	math::Vector3Int const& chunk() const { return this->mChunkPosition; }
	math::Vector3Int const& local() const { return this->mBlockPosition; }
	math::Vector3 const& offset() const { return this->mBlockOffset; }
	math::Vector3Int& chunk() { return this->mChunkPosition; }
	math::Vector3Int& local() { return this->mBlockPosition; }
	math::Vector3& offset() { return this->mBlockOffset; }
	
	math::Vector<f64, 3> toGlobal() const;
	static Coordinate fromGlobal(math::Vector<f64, 3> global);

	bool operator==(Coordinate const& other) const;
	bool operator!=(Coordinate const& other) const;
	void operator=(Coordinate const &other);
	Coordinate& operator+=(Coordinate const &other);
	Coordinate& operator+=(math::Vector3 const &other);
	Coordinate const operator+(Coordinate const &other) const;
	void operator-=(Coordinate const &other);
	Coordinate const operator-(Coordinate const &other) const;
	Coordinate const operator+(math::Vector3Int const &other) const;
	Coordinate const operator-(math::Vector3Int const &other) const;

	/**
	 * Comparison operator for handling 
	 * TODO: Eventually replace with the spaceship (<=>) operator
	 * https://devblogs.microsoft.com/cppblog/simplify-your-code-with-rocket-science-c20s-spaceship-operator/
	 */
	bool operator<(Coordinate const& other) const;

	std::string toString() const;

private:
	math::Vector3Int mChunkPosition;
	math::Vector3Int mBlockPosition;
	math::Vector3 mBlockOffset;

	void adjustForChunkSize();

};

NS_END

#include "network/data/NetworkDataBuffer.hpp"

NS_NETWORK
void write(Buffer &buffer, std::string name, world::Coordinate value);
void read(Buffer &buffer, std::string name, world::Coordinate &value);
NS_END