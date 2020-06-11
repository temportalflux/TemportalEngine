#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "thread/MutexLock.hpp"
#include "memory/MemoryManager.h"

#include <memory>
#include <cassert>

NS_MEMORY

#pragma region Std-Wrapper
NS_INTERNAL

template <typename T>
struct allocator_wrapper
{
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	template <typename U>
	struct rebind
	{
		typedef allocator_wrapper<U> other;
	};

	void* mpMemoryManager;
	thread::MutexLock *mpLock;
	uSize mSubChunkSize;

	// subChunkSize should ONLY be used if the allocator is making a chunk within a chunk
	allocator_wrapper(void* pMemoryManager, thread::MutexLock *pLock, uSize subChunkSize=0)
	{
		this->mpMemoryManager = pMemoryManager;
		this->mpLock = pLock;
		this->mSubChunkSize = subChunkSize;
	}

	allocator_wrapper() throw() {}
	allocator_wrapper(allocator_wrapper const &other)
	{
		this->mpMemoryManager = other.mpMemoryManager;
		this->mpLock = other.mpLock;
		this->mSubChunkSize = other.mSubChunkSize;
	}

	template <typename U>
	allocator_wrapper(allocator_wrapper<U> const &other)
	{
		this->mpMemoryManager = other.mpMemoryManager;
		this->mpLock = other.mpLock;
		this->mSubChunkSize = other.mSubChunkSize;
	}

	template <typename U>
	allocator_wrapper& operator=(allocator_wrapper<U> const &other) { return *this; }
	allocator_wrapper<T>& operator=(allocator_wrapper const &other) { return *this; }

	~allocator_wrapper() {}

	pointer allocate(size_type n, const void* hint = 0)
	{
		void* ptr = nullptr;
		this->mpLock->lock();
		uSize size = (n * sizeof(T)) + this->mSubChunkSize;
		a3_mem_manager_alloc(this->mpMemoryManager, size, &ptr);
		this->mpLock->unlock();
		return static_cast<T*>(ptr);
	}

	void deallocate(T* ptr, size_type n)
	{
		this->mpLock->lock();
		a3_mem_manager_dealloc(this->mpMemoryManager, ptr);
		this->mpLock->unlock();
	}

};

template <typename T, typename U>
inline bool operator==(allocator_wrapper<T> const &a, allocator_wrapper<U> const &b) { return true; }
template <typename T, typename U>
inline bool operator!=(allocator_wrapper<T> const &a, allocator_wrapper<U> const &b) { return !(a == b); }

NS_END
#pragma endregion

class MemoryChunk
{

public:

	static std::shared_ptr<MemoryChunk> Create(uSize chunkSize)
	{
		auto ptr = std::make_shared<MemoryChunk>();
		ptr->bUsedMalloc = true;
		ptr->mpMemoryManager = malloc(chunkSize);
		a3_mem_manager_init(ptr->mpMemoryManager, chunkSize);
		return ptr;
	}

	MemoryChunk()
	{
		this->mpMemoryManager = nullptr;
	}

	~MemoryChunk()
	{
		if (this->bUsedMalloc)
		{
			assert(this->mpMemoryManager != nullptr);
			free(this->mpMemoryManager);
		}
	}

	std::shared_ptr<MemoryChunk> createChunk(uSize chunkSize)
	{
		auto internalAllocator = memory::_internal::allocator_wrapper<MemoryChunk>(
			this->mpMemoryManager, &this->mLock, chunkSize
		);
		auto chunk = std::allocate_shared<MemoryChunk>(internalAllocator);
		chunk->bUsedMalloc = false;
		// chunk being managed will be exactly 1 stride away from memory location of the MemoryChunk itself
		// so adding 1 to the pointer will result in a pointer to whatever memory is immediately after the memory chunk
		chunk->mpMemoryManager = (chunk.get() + 1);
		a3_mem_manager_init(chunk->mpMemoryManager, chunkSize);
		return chunk;
	}

	template <typename T, typename... Types>
	std::shared_ptr<T> make_shared(Types&& ...args)
	{
		auto internalAllocator = memory::_internal::allocator_wrapper<T>(
			this->mpMemoryManager, &this->mLock
		);
		return std::allocate_shared<T>(internalAllocator, args...);
	}

	template <typename T>
	T* allocate()
	{
		void* ptr = nullptr;
		this->mLock.lock();
		uSize size = sizeof(T);
		a3_mem_manager_alloc(this->mpMemoryManager, size, &ptr);
		this->mLock.unlock();
		return static_cast<T*>(ptr);
	}

	template <typename T>
	void deallocate(T* ptr)
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
