#pragma once

#include "TemportalEnginePCH.hpp"

#include "thread/MutexLock.hpp"
#include "memory/MemoryManager.h"

// memory::MemoryChunk::Create
NS_MEMORY

/**
 * A `MemoryChunk` where all memory allocations are of the same type/size.
 * TODO: REDO THIS CLASS. MemoryPools should not use the memory manager.
		Their object size is fixed (no need for node headers), and the only metadata it needs is how many
		objects are allocated, so long as they are all pushed to the front of the manager. Can also store
		an array of id when inserted (index) to current index so the id returned from allocate is always valid.
 */
template <typename TObject, uSize Count>
class MemoryPool
{
	constexpr static uSize chunkSize()
	{
		return a3_mem_manager_totalChunkSize(Count) + Count * allocationSize();
	}

	constexpr static uSize allocationSize()
	{
		return sizeof(TObject);
	}

public:
	static std::shared_ptr<MemoryPool<TObject, Count>> create()
	{
		auto ptr = std::make_shared<MemoryPool<TObject, Count>>();
		ptr->bUsedMalloc = true;
		ptr->mpMemoryManager = malloc(chunkSize());
		a3_mem_manager_init(ptr->mpMemoryManager, chunkSize());
		return ptr;
	}

	MemoryPool()
	{
		this->mpMemoryManager = nullptr;
		this->mAllocatedCount = 0;
		//this->mAllocatedIndicies.fill(0);
	}

	~MemoryPool()
	{
		if (this->bUsedMalloc && this->mpMemoryManager != nullptr)
		{
			assert(this->mpMemoryManager != nullptr);
			free(this->mpMemoryManager);
			this->mpMemoryManager = nullptr;
		}
	}

	constexpr uSize size() const { return Count; }

	class iterator : public std::iterator<std::forward_iterator_tag, TObject>
	{
	public:

		explicit iterator(MemoryPool<TObject, Count> &pool, uSize idx = 0)
			: mIdxAllocatedIndex(idx), mPool(pool)
		{
		}
		
		// Dereference iterator to the editable memory
		TObject& operator*() const
		{
			ui8 isAllocated;
			auto idx = this->mPool.mAllocatedIndicies[this->mIdxAllocatedIndex];
			auto ptr = a3_mem_manager_atUniformIndex(this->mPool.mpMemoryManager, allocationSize(), idx, &isAllocated);
			if (!isAllocated) throw std::logic_error("iterator points to unallocated memory");
			return *reinterpret_cast<TObject*>(ptr);
		}

		// Increment iterator to next allocated location (prefix operator)
		iterator& operator++()
		{
			this->mIdxAllocatedIndex++;
			return *this;
		}

		// Postfix operator for incrementing the iterator
		iterator& operator++(i32 i)
		{
			return ++(*this);
		}

		bool operator!=(iterator const &other) const
		{
			return this->mIdxAllocatedIndex != other.mIdxAllocatedIndex;
		}

	private:
		uSize mIdxAllocatedIndex; // index of an allocated index in mAllocatedIndicies
		MemoryPool<TObject, Count> &mPool;

	};

	iterator begin()
	{
		return this->mAllocatedCount > 0 ? iterator(*this, this->mAllocatedIndicies[0]) : this->end();
	}

	iterator end()
	{
		return iterator(*this, this->mAllocatedCount);
	}

	// Returns the index of the item in the pool
	template <typename... TArgs>
	iterator allocate(TArgs&& ...args)
	{
		uSize idx;
		this->mLock.lock();
		{
			void* ptr = nullptr;
			uSize size = allocationSize();
			bool wasAllocated = a3_mem_manager_alloc(this->mpMemoryManager, size, &ptr) > 0;
			assert(wasAllocated);

			idx = a3_mem_manager_indexOfPtr(this->mpMemoryManager, ptr, size);
			this->markAllocated(idx, true);
		}
		this->mLock.unlock();
		return iterator(*this, idx);
	}

	void deallocate(uSize idx)
	{
		this->mLock.lock();
		{
			a3_mem_manager_deallocUniform(this->mpMemoryManager, idx, allocationSize());
			this->markAllocated(idx, false);
		}
		this->mLock.unlock();
	}

	void markAllocated(uSize idxAllocated, bool allocated)
	{
		if (allocated)
		{
			uSize idxAllocatedIndex = 0;
			while (
				idxAllocatedIndex < this->mAllocatedCount &&
				this->mAllocatedIndicies[idxAllocatedIndex] < idxAllocated
			) idxAllocatedIndex++;
			// idxAllocatedIndex is now at the position where idxAllocated should go
			// Shift the remaining items by copying them to the next position
			auto dataPtr = this->mAllocatedIndicies.data();
			memcpy(dataPtr + idxAllocatedIndex + 1, dataPtr + idxAllocatedIndex, (this->mAllocatedCount - idxAllocatedIndex) * sizeof(uSize));
			this->mAllocatedIndicies[idxAllocatedIndex] = idxAllocated;
			this->mAllocatedCount++;
		}
		else
		{
			uSize idxAllocatedIndex = 0;
			while (
				idxAllocatedIndex < this->mAllocatedCount &&
				this->mAllocatedIndicies[idxAllocatedIndex] != idxAllocated
			) idxAllocatedIndex++;
			// copy everything after allocated idx to shift it up by one
			auto dataPtr = this->mAllocatedIndicies.data();
			memcpy(dataPtr + idxAllocatedIndex, dataPtr + idxAllocatedIndex + 1, (this->mAllocatedCount - idxAllocatedIndex - 1) * sizeof(uSize));
			this->mAllocatedCount--;
		}
	}

private:
	bool bUsedMalloc;

	// Thread-safe lock to prevent multiple threads from editing the chunk at once.
	// Locks the memory manager so no other allocations or deallocations can happen.
	// Important to retain the state of the free node tree.
	thread::MutexLock mLock;

	// The chunk of memory that is managed.
	// MemoryChunk will only allocate things to this block of memory
	void* mpMemoryManager;

	// Number of allocated items in the pool (may not be adjacent in memory)
	uSize mAllocatedCount;

	std::array<uSize, Count> mAllocatedIndicies;

};

NS_END
