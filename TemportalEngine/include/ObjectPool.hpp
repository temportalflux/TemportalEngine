#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedHashMap.hpp"
#include "utility/Guid.hpp"
#include "memory/MemoryPool.hpp"
#include "thread/MutexLock.hpp"

template <typename TIdentifier, typename TObject, uSize Capacity>
class ObjectPool
{

public:
	constexpr uSize capacity() const { return Capacity; }

	uSize size() const { return this->mIdToMemoryIdx.size(); }

	ObjectPool() = default;

	template <typename... TArgs>
	TObject* create(TIdentifier const& id, TArgs... args)
	{
		this->mLock.lock();
		auto idxInMemory = this->mMemory.allocate(args...);
		this->mIdToMemoryIdx.insert(id, idxInMemory);
		this->mLock.unlock();
		return &this->mMemory[idxInMemory];
	}
	
	// TODO: Can this be done in batches to limit the amount of decrementation iteration over FixedHashMap? Currently this is O(n) because of that operation
	void destroy(TIdentifier const &id)
	{
		this->mLock.lock();
		
		// Collapse that item in memory
		auto lookupPtr = this->mIdToMemoryIdx.lookup(id);
		if (lookupPtr == nullptr)
		{
			this->mLock.unlock();
			return;
		}
		auto idxInMemory = *lookupPtr;
		this->mMemory.deallocate(idxInMemory);
		
		// Remove it from the map
		this->mIdToMemoryIdx.remove(id);

		// Update indicies
		// all indicies must be updated now because their indicies were changed in mMemory.
		// Everything with a mem index > idxInMemory must be decremented.
		for (uSize idx = 0; idx < this->mIdToMemoryIdx.size(); ++idx)
		{
			auto& memoryIdx = this->mIdToMemoryIdx[idx];
			if (memoryIdx > idxInMemory) memoryIdx--;
		}

		this->mLock.unlock();
	}

	TObject* lookup(TIdentifier const &id)
	{
		auto lookupPtr = this->mIdToMemoryIdx.lookup(id);
		if (lookupPtr == nullptr) return nullptr;
		return &this->mMemory[*lookupPtr];
	}

private:
	thread::MutexLock mLock;
	// NOTE: Keeping track of ids like this nearly doubles the size of the pool depending on the object and identifier
	// i.e. Capacity of 1024 for a 28 byte structure (3 floats + id) takes a total pool size of 53272, 24584 of which is this map (if using guids)
	// Contains 2 arrays of length Capacity. If TIdentifier=ui16 and this is a x64 build (uSize=ui64), thats (2 + 16 bytes)*1024 = 18432 bytes (+ size count)
	FixedHashMap<TIdentifier, uSize, Capacity> mIdToMemoryIdx;
	memory::MemoryPool<TObject, Capacity> mMemory;

};
