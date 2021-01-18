#pragma once

#include "memory/MemoryPool.hpp"

#include "thread/MutexLock.hpp"

class ObjectPool : public memory::Pool
{

public:
	ObjectPool() : memory::Pool() {}
	ObjectPool(uSize objectSize, uSize capacity)
		: memory::Pool(objectSize, capacity)
	{}

	void* create(uIndex &outId)
	{
		this->mLock.lock();
		outId = this->allocate();
		this->mLock.unlock();
		return this->at(outId);
	}

	void destroy(uIndex const &id)
	{
		this->mLock.lock();
		this->deallocate(id);
		this->mLock.unlock();
	}

	template <typename TObject, typename... TArgs>
	TObject* create(uIndex &outId, TArgs... args)
	{
		TObject* ptr = reinterpret_cast<TObject*>(
			this->create(outId)
		);
		new (ptr) TObject(args...);
		return ptr;
	}

	template <typename TObject>
	TObject* lookup(uIndex const &id)
	{
		return reinterpret_cast<TObject*>(this->at(id));
	}
	
	template <typename TObject>
	void destroy(uIndex const &id)
	{
		auto pObject = this->lookup<TObject>(id);
		pObject->~TObject();
		this->destroy(id);
	}

private:
	thread::MutexLock mLock;

};
