#pragma once

#include "TemportalEnginePCH.hpp"

NS_MEMORY

template <typename TObject, uSize Capacity>
class MemoryPool
{

public:
	
	constexpr uSize capacity() const { return Capacity; }

	uSize size() const { return this->mSize; }

	uSize memSize() const { return capacity() * sizeof(TObject); }

	MemoryPool()
	{
		this->mSize = 0;
		memset(this->mValues, 0, memSize());
		this->mEmptySlotCount = 0;
		memset(this->mEmptySlots, 0, sizeof(mEmptySlots));
	}

	template <typename... TArgs>
	uIndex allocate(TArgs... args)
	{
		assert(size() < capacity());
		uIndex idx = this->mSize;
		if (this->mEmptySlotCount > 0)
		{
			idx = this->mEmptySlots[--this->mEmptySlotCount];
		}
		new (&this->mValues[idx]) TObject(args...);
		this->mSize++;
		return idx;
	}

	void deallocate(uSize idx)
	{
		assert(this->mEmptySlotCount < capacity());
		this->mEmptySlots[this->mEmptySlotCount++] = idx;
		memset(this->mValues + idx, 0, sizeof(TObject));
		this->mSize--;
	}

	TObject& operator[](uSize idx)
	{
		return this->mValues[idx];
	}

private:	
	TObject mValues[Capacity];
	uSize mSize;

	uIndex mEmptySlots[Capacity];
	uSize mEmptySlotCount;

};

NS_END
