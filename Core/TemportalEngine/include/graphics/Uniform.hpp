#pragma once

#include "TemportalEnginePCH.hpp"

#include "memory/MemoryChunk.hpp"
#include "thread/MutexLock.hpp"

NS_GRAPHICS

class Uniform
{
public:

	template <typename T>
	static std::shared_ptr<Uniform> create(std::shared_ptr<memory::MemoryChunk> chunk)
	{
		auto ptr = chunk->make_shared<Uniform>();
		ptr->mpMemoryChunk = chunk;
		ptr->mDataSize = (uSize)sizeof(T);
		ptr->mpData = chunk->allocate<T>();
		new (ptr->mpData)T();
		return ptr;
	}

	Uniform() : mpData(nullptr), bHasChangedSinceLastRead(false) {}
	~Uniform()
	{
		if (this->mpData != nullptr)
		{
			this->mpMemoryChunk->deallocate(this->mpData);
			this->mpData = nullptr;
			this->mpMemoryChunk.reset();
		}
	}

	void beginReading()
	{
		this->mLock.lock();
	}

	void endReading(bool markAsSeen)
	{
		if (markAsSeen) this->bHasChangedSinceLastRead = false;
		this->mLock.unlock();
	}

	template <typename T>
	T read()
	{
		OPTICK_EVENT();
		assert(this->mDataSize == (uSize)sizeof(T));
		this->beginReading();
		T copiedData = *((T*)this->mpData);
		this->endReading(false);
		return copiedData;
	}

	void* data() const { return this->mpData; }
	uSize size() const { return this->mDataSize; }

	template <typename T>
	void write(T *data)
	{
		OPTICK_EVENT();
		assert(this->mpData != nullptr);
		assert(this->mDataSize == (uSize)sizeof(T));
		this->mLock.lock();
		memcpy(this->mpData, data, this->mDataSize);
		this->bHasChangedSinceLastRead = true;
		this->mLock.unlock();
	}

	bool hasChanged() const
	{
		return this->bHasChangedSinceLastRead;
	}

private:
	std::shared_ptr<memory::MemoryChunk> mpMemoryChunk;

	/**
	 * Raw pointer to the location where the data for this uniform is stored on the CPU.
	 */
	void* mpData;

	/**
	 * The size of the data pointed at by `mpData`
	 */
	uSize mDataSize;

	thread::MutexLock mLock;
	bool bHasChangedSinceLastRead;

};

NS_END
