#pragma once

#include "TemportalEnginePCH.hpp"

#include "thread/MutexLock.hpp"
#include "memory/MemoryManager.h"

// memory::MemoryChunk::Create
NS_MEMORY

/**
 * A `MemoryChunk` where all memory allocations are of the same type/size.
 */
template <typename TObject, uSize Count>
class MemoryPool
{
	constexpr static uSize chunkSize()
	{
		return a3_mem_manager_totalChunkSize(Count) + Count * allocationSize();
	}

	constexpr static uSize allocationHeaderSize()
	{
		// std::allocate_shared wraps the TObject in some std::_Ref_count_obj_alloc template
		// This template is not available at compile time, but is 40 bytes on x64 machines
#ifdef _WIN64
		return 40;
#elif _WIN32
		return 24;
#else
		return 0;
#endif
	}

	constexpr static uSize allocationSize()
	{
		return allocationHeaderSize() + sizeof(TObject);
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

	template <typename... Types>
	std::shared_ptr<TObject> make_shared(Types&& ...args)
	{
		auto internalAllocator = memory::_internal::allocator_wrapper<TObject>(
			this->mpMemoryManager, &this->mLock
		);
		return std::allocate_shared<TObject>(internalAllocator, args...);
	}

	uSize indexOf(std::shared_ptr<TObject> ptr)
	{
		uSize raw = (uSize)ptr.get();
		raw -= allocationHeaderSize();
		return a3_mem_manager_indexOfPtr(this->mpMemoryManager, (void*)raw, allocationSize());
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
