#pragma once

#include "types/integer.h"
#include "types/real.h"

template <typename TValue>
struct TValueIterator
{
public:
	TValueIterator(TValue* data, uIndex offset) : mData(data), mOffset(offset) {}
	TValueIterator(TValueIterator<TValue> const& other) : mData(other.mData), mOffset(other.mOffset) {}

	TValue& operator*() { return *(this->mData + this->mOffset); }

	void operator++() { this->mOffset++; }

	bool operator!=(TValueIterator<TValue> const& other) { return this->mOffset != other.mOffset; }

private:
	TValue* mData;
	uIndex mOffset;

};
