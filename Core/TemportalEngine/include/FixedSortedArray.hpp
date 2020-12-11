#pragma once

#include "TemportalEnginePCH.hpp"
#include "dataStructures/ValueIterator.hpp"

template <typename TValue, uSize Capacity>
class FixedSortedArray
{

public:
	typedef TValueIterator<TValue> iterator;
	typedef TValueIterator<TValue const> const_iterator;
	inline constexpr uSize capacity() const { return Capacity; }

	uSize size() const { return this->mSize; }

	FixedSortedArray() : mSize(0)
	{
		memset(this->mValues, 0, sizeof(TValue) * capacity());
	}

	uIndex insert(TValue const &value)
	{
		assert(size() < capacity());
		auto desiredIdx = this->findDesiredIdx(value);
		for (auto idxValue = this->mSize; idxValue > desiredIdx; --idxValue)
		{
			this->mValues[idxValue] = this->mValues[idxValue - 1];
		}
		this->mValues[desiredIdx] = value;
		this->mSize++;
		return desiredIdx;
	}

	uIndex push(TValue const &value)
	{
		assert(size() < capacity());
		uIndex idx = this->mSize;
		this->mValues[idx] = value;
		this->mSize++;
		return idx;
	}

	TValue remove(uIndex const& idx)
	{
		assert(size() > 0 && idx < size());
		TValue value = this->mValues[idx];
		this->mSize--;
		for (auto idxValue = idx; idxValue < this->mSize; ++idxValue)
		{
			this->mValues[idxValue] = this->mValues[idxValue + 1];
		}
		this->mValues[this->mSize] = TValue();
		return value;
	}

	TValue dequeue() { return remove(0); }
	TValue pop() { return remove(size() - 1); }

	void clear()
	{
		for (uIndex idx = 0; idx < size(); ++idx)
		{
			this->mValues[idx].~TValue();
			memset(this->mValues + idx, 0, sizeof(TValue));
		}
	}

	std::optional<uIndex> search(std::function<i8(TValue const& value)> predicate)
	{
		auto foundEntry = this->findIndex(predicate);
		if (foundEntry.first) return foundEntry.second;
		else return std::nullopt;
	}

	constexpr TValue& operator[](uIndex const i) { return mValues[i]; }
	constexpr TValue const& operator[](uIndex const i) const { return mValues[i]; }

	iterator begin() { return iterator(mValues, 0); }
	iterator end() { return iterator(mValues, mSize); }
	const_iterator begin() const { return const_iterator(mValues, 0); }
	const_iterator end() const { return const_iterator(mValues, mSize); }

private:
	uSize mSize;
	TValue mValues[Capacity];

	uIndex findDesiredIdx(TValue const &value) const
	{
		auto foundIdx = findIndex([value](TValue const& entry)
		{
			// value <=> entry
			return value < entry ? -1 : (value > entry ? 1 : 0);
		});
		return foundIdx.second;
	}

	std::pair<bool, uIndex> findIndex(std::function<i8(TValue const& value)> predicate) const
	{
		i64 startIndex = 0, endIndex = mSize - 1;
		while (startIndex <= endIndex)
		{
			i64 middle = startIndex + (endIndex - startIndex) / 2;
			auto comp = predicate(mValues[middle]);
			if (comp < 0)
			{
				endIndex = middle - 1;
				if (startIndex > endIndex) return std::make_pair(false, middle - 1);
			}
			else if (comp > 0)
			{
				startIndex = middle + 1;
				if (startIndex > endIndex) return std::make_pair(false, middle + 1);
			}
			else return std::make_pair(true, middle);
		}
		return std::make_pair(false, startIndex);
	}

};
