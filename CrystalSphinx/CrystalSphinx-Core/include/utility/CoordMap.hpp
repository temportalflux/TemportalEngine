#pragma once

#include "CoreInclude.hpp"

#include "dataStructures/ValueIterator.hpp"

template <typename TValue, uSize TSize>
class CoordMap;

template <typename TOwner, uSize TSize, typename TIterValue>
struct TCoordIterator
{
	typedef math::Vector3Int TCoordinate;
public:
	TCoordIterator(TOwner *data, TCoordinate const& coord) : mpData(data), mCoord(coord) {}
	TCoordIterator(TCoordIterator const& other) : mpData(other.mpData), mCoord(other.mCoord) {}

	struct TResult
	{
		TCoordinate localCoord;
		TIterValue &data;
	};

	TResult operator*() { return { mCoord, (*mpData)[mCoord] }; }

	void operator++()
	{
		mCoord.x() += 1;
		if (mCoord.x() >= TSize)
		{
			mCoord.z() += 1;
			mCoord.x() = 0;
		}
		if (mCoord.z() >= TSize)
		{
			mCoord.y() += 1;
			mCoord.z() = 0;
		}
	}

	bool operator!=(TCoordIterator const& other) { return this->mCoord != other.mCoord; }

private:
	TOwner *mpData;
	TCoordinate mCoord;

};

template <typename TValue, uSize TSize>
class CoordMap
{
	static constexpr math::Vector3Int max() { return math::Vector3Int{ 0, TSize, 0 }; }

public:
	typedef CoordMap<TValue, TSize> TSelf;
	typedef math::Vector3Int TCoordinate;
	
	typedef TCoordIterator<TSelf, TSize, TValue> iterator;
	typedef TCoordIterator<TSelf const, TSize, TValue const> const_iterator;

	TValue const& operator[](TCoordinate const& coord) const
	{
		assert(coord.x() < TSize && coord.y() < TSize && coord.z() < TSize);
		return mValues[coord.z()][coord.y()][coord.x()];
	}

	TValue& operator[](TCoordinate const& coord)
	{
		assert(coord.x() < TSize && coord.y() < TSize && coord.z() < TSize);
		return mValues[coord.z()][coord.y()][coord.x()];
	}

	iterator begin() { return iterator(this, { 0, 0, 0 }); }
	iterator end() { return iterator(this, max()); }
	const_iterator begin() const { return const_iterator(this, { 0, 0, 0 }); }
	const_iterator end() const { return const_iterator(this, max()); }

private:
	std::array<std::array<std::array<TValue, TSize>, TSize>, TSize> mValues;
	
};
