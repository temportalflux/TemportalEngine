#pragma once

#include "TemportalEnginePCH.hpp"

template <typename TValue, uSize Capacity>
class FixedCircularQueue
{

public:
	constexpr uSize capacity() const { return Capacity; }

	// TODO: Can this be computed by comparing head and tail indicies?
	uSize size() const { return this->mSize; }

	FixedCircularQueue() : mSize(0), mIdxHead(0), mIdxTail(0)
	{
		memset(this->mValues, 0, sizeof(TValue) * capacity());
	}

	uIndex enqueue(TValue const &value)
	{
		// If tail == head, then size==capacity
		assert(this->mIdxTail != this->mIdxHead);
		auto idx = this->mIdxTail++;
		this->mValues[idx] = value;
		this->mSize++;
		return idx;
	}

	// Always removes from the start of the queue
	TValue dequeue()
	{
		assert(this->mSize > 0);
		this->mSize--;
		// dequeueing is just moving the head past the node
		return this->mValues[this->mIdxHead++];
	}

private:
	uSize mSize;
	// The index of the first node with a value
	uIndex mIdxHead;
	// The index after the last node with a value
	uIndex mIdxTail;
	TValue mValues[Capacity];

};
