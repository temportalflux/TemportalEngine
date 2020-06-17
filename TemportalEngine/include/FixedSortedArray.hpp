#pragma once

#include "TemportalEnginePCH.hpp"

template <typename TValue, uSize Capacity>
class FixedSortedArray
{

public:
	constexpr uSize capacity() const { return Capacity; }

	uSize size() const { return this->mSize; }

	FixedSortedArray() : mSize(0)
	{
		memset(this->mValues, 0, sizeof(TValue) * capacity());
	}

	uIndex insert(TValue const &value)
	{
		auto desiredIdx = this->find(value);
		memcpy(this->mValues + desiredIdx + 1, this->mValues + desiredIdx, sizeof(TValue) * (this->mSize - desiredIdx));
		this->mValues[desiredIdx] = value;
		this->mSize++;
		return desiredIdx;
	}

	TValue dequeue()
	{
		assert(size() > 0);
		TValue value = this->mValues[0];
		memcpy(this->mValues, this->mValues + 1, sizeof(TValue) * (this->mSize - 1));
		this->mSize--;
		return value;
	}

private:
	uSize mSize;
	TValue mValues[Capacity];

	uIndex find(TValue const &value) const
	{
		uIndex idx, first = 0;
		uSize count = this->mSize, step;
		while (count > 0)
		{
			idx = first;
			step = count / 2;
			idx += step;
			if (this->mValues[idx] < value)
			{
				first = idx + 1;
				count -= step + 1;
			}
			else
				count = step;
		}
		return first;
	}

};
