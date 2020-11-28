#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedHashMap.hpp"
#include "utility/Guid.hpp"
#include "memory/MemoryPool.hpp"
#include "thread/MutexLock.hpp"

template <typename TObject, uSize Capacity>
class ObjectPool
{

public:
	constexpr uSize capacity() const { return Capacity; }

	uSize size() const { return this->mMemory.size(); }

	ObjectPool() = default;

	template <typename... TArgs>
	TObject* create(uIndex &outId, TArgs... args)
	{
		this->mLock.lock();
		outId = this->mMemory.allocate(args...);
		this->mLock.unlock();
		return &this->mMemory[outId];
	}
	
	void destroy(uIndex const &id)
	{
		this->mLock.lock();
		((TObject*)(&this->mMemory[id]))->~TObject();
		this->mMemory.deallocate(id);
		this->mLock.unlock();
	}

	TObject* lookup(uIndex const &id)
	{
		return &this->mMemory[id];
	}

private:
	thread::MutexLock mLock;
	memory::MemoryPool<TObject, Capacity> mMemory;

};
