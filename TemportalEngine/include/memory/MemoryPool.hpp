#pragma once

#include "TemportalEnginePCH.hpp"

#include "thread/MutexLock.hpp"
#include "memory/MemoryManager.h"

// memory::MemoryChunk::Create
NS_MEMORY

/**
 * A `MemoryChunk` where all memory allocations are of the same type/size.
 */
template <typename TObject, ui64 Count>
class MemoryPool
{
	constexpr static ui64 chunkSize() { return a3_mem_manager_totalChunkSize(Count) + sizeof(TObject) * Count; }

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
	}

	~MemoryPool()
	{
		if (this->bUsedMalloc)
		{
			assert(this->mpMemoryManager != nullptr);
			free(this->mpMemoryManager);
		}
	}

	template <typename... Types>
	std::shared_ptr<TObject> make_shared(Types&& ...args)
	{
		auto internalAllocator = memory::_internal::allocator_wrapper<TObject>(
			this->mpMemoryManager, &this->mLock
		);
		return std::allocate_shared<TObject>(internalAllocator, args...);
	}

	TObject* allocate()
	{
		void* ptr = nullptr;
		this->mLock.lock();
		uSize size = sizeof(TObject);
		a3_mem_manager_alloc(this->mpMemoryManager, size, &ptr);
		this->mLock.unlock();
		return static_cast<TObject*>(ptr);
	}

	void deallocate(TObject* ptr)
	{
		this->mLock.lock();
		a3_mem_manager_dealloc(this->mpMemoryManager, ptr);
		this->mLock.unlock();
	}

private:
	bool bUsedMalloc;

	// Thread-safe lock to prevent multiple threads from editing the chunk at once.
	thread::MutexLock mLock;

	// The chunk of memory that is managed.
	// MemoryChunk will only allocate things to this block of memory
	void* mpMemoryManager;

};

NS_END
