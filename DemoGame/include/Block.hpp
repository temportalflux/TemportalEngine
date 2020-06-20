#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

/*
	Persistent Properties:
	- Owning Chunk Coordinate
	- Coordinate in Chunk
	Generated Properties:
	- 6-bit mask (1-bit per side) which stores if that face is visible
		Derived based on if the adjacent sides are occupied
*/

class Block
{

private:

	// 0 = block not visible
	ui8 mSideVisibility;

	bool isSideVisible(math::Vector3Int const &dir) const
	{
		auto mask = dir.toBitMask();
		return this->mSideVisibility & mask == mask;
	}

	void setSideVisibility(math::Vector3Int const &dir, bool isVisible)
	{
		this->mSideVisibility = isVisible
			? (this->mSideVisibility | dir.toBitMask())
			: (this->mSideVisibility & ~dir.toBitMask());
	}

	void toggleSideVisibility(math::Vector3Int const &dir)
	{
		this->mSideVisibility ^= dir.toBitMask();
	}

};
