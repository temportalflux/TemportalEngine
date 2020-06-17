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
	}

	template <typename... TArgs>
	uSize allocate(TArgs... args)
	{
		assert(size() < capacity());
		uSize idx = this->mSize;
		new (&this->mValues[idx]) TObject(args...);
		this->mSize++;
		return idx;
	}

	void deallocate(uSize idx)
	{
		// Shift all memory to collapse over that element
		memcpy(this->mValues + idx, this->mValues + idx + 1, sizeof(TObject) * (this->mSize - idx - 1));
		this->mSize--;
	}

	TObject& operator[](uSize idx)
	{
		return this->mValues[idx];
	}

private:	
	TObject mValues[Capacity];
	uSize mSize;

};

NS_END
