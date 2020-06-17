#pragma once

#include "TemportalEnginePCH.hpp"

/**
 * A fixed-size hashable map which stores 2 arrays, one for the keys and one for values.
 */
template <typename TKey, typename TValue, uSize Capacity>
class FixedHashMap
{

public:

	FixedHashMap()
	{
		memset(this->mKeys, 0, capacity() * sizeof(TKey));
		memset(this->mValues, 0, capacity() * sizeof(TValue));
	}

	constexpr uSize capacity() const { return Capacity; }

	uSize size() const { return this->mSize; }

	/**
	 * Performs an insertion-sort of the key, and maps the key to the value for lookup.
	 */
	void insert(TKey const &key, TValue const &value)
	{
		if (this->mSize >= Capacity) throw std::out_of_range("exceeded capacity");

		uSize desiredIdx = this->find(key);
		// Shift memory from [desiredIdx, mSize - desiredIdx] -> [desiredIdx + 1, mSize - desiredIdx + 1]
		// effectively: copy range [2, 5] -> [3, 6]
		// which translates into src=desiredIdx dest=desiredIdx+1 count=mSize-desiredIdx
		auto srcIdx = desiredIdx, destIdx = desiredIdx + 1;
		auto count = this->mSize - srcIdx;
		memcpy(this->mKeys + destIdx, this->mKeys + srcIdx, sizeof(TKey) * count);
		memcpy(this->mValues + destIdx, this->mValues + srcIdx, sizeof(TValue) * count);

		this->mKeys[desiredIdx] = key;
		this->mValues[desiredIdx] = value;
		this->mSize++;
	}

	// O(log(n)) binary-search
	// Precondition: TKey implements operator==(TKey other)
	bool contains(TKey const &key) const
	{
		return key == this->mKeys[this->find(key)];
	}

	void remove(TKey const &key)
	{
		uSize desiredIdx = this->find(key);
		assert(key == this->mKeys[desiredIdx]);
		// Perform opposite shift as insert does
		auto srcIdx = desiredIdx + 1, destIdx = desiredIdx;
		auto count = this->mSize - srcIdx;
		memcpy(this->mKeys + destIdx, this->mKeys + srcIdx, sizeof(TKey) * count);
		memcpy(this->mValues + destIdx, this->mValues + srcIdx, sizeof(TValue) * count);
		this->mSize--;
	}

	/**
	 * Performs a binary-search lookup of the mapping from key to value, and returns the value.
	 * Will compare based on hash, and if there is a collision, compares string values.
	 * Precondition: TKey is hashable and has a `std::string(const TKey&)` casting operator.
	 */
	TValue* lookup(TKey const &key)
	{
		uSize idx = this->find(key);
		return key == this->mKeys[idx] ? &this->mValues[idx] : nullptr;
	}

	TKey& at(uSize const &idx)
	{
		assert(idx < size());
		return this->mKeys[idx];
	}
	
	TValue& operator[](uSize const &idx)
	{
		assert(idx < size());
		return this->mValues[idx];
	}

private:
	TKey mKeys[Capacity];
	TValue mValues[Capacity];
	// Amount of mappings actually stored
	uSize mSize;

	/**
	 * Returns true if `std::hash(a) < std:hash(b)` or `((std::string)a).compare((std::string)b) < 0`.
	 */
	bool compareKeyLess(TKey const &a, TKey const &b) const
	{
		auto hasher = std::hash<TKey>();
		auto aHash = hasher(a), bHash = hasher(b);
		if (aHash < bHash) return true;
		else if (aHash > bHash) return false;
		else return ((std::string)a).compare((std::string)b) < 0;
	}

	uSize find(TKey const &key) const
	{
		uSize idx, first = 0;
		uSize count = this->mSize, step;
		while (count > 0)
		{
			idx = first;
			step = count / 2;
			idx += step;
			if (this->compareKeyLess(this->mKeys[idx], key))
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
