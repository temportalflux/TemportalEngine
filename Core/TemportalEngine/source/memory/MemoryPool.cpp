#include "memory/MemoryPool.hpp"

using namespace memory;

Pool::Pool()
	: mObjectSize(0)
	, mCapacity(0)
	, mpData(nullptr)
	, mSize(0)
	, mbOwnsMemory(false)
{

}

Pool::Pool(uSize objectSize, uSize capacity) : Pool()
{
	this->mObjectSize = objectSize;
	this->mCapacity = capacity;
}

Pool::~Pool()
{
	if (this->mbOwnsMemory && this->mpData != nullptr)
	{
		free(this->mpData);
		this->mbOwnsMemory = false;
		this->mpData = nullptr;
	}
}

Pool& Pool::init(uSize objectSize, uSize capacity)
{
	this->mObjectSize = objectSize;
	this->mCapacity = capacity;
	return *this;
}

uSize Pool::capacity() const { return this->mCapacity; }
uSize Pool::size() const { return this->mSize; }
uSize Pool::objectSize() const { return this->mObjectSize; }
uSize Pool::memSize() const { return this->capacity() * this->objectSize(); }

void Pool::allocateMemory()
{
	assert(this->mpData == nullptr);
	this->mbOwnsMemory = true;
	this->assignMemory(malloc(this->memSize()));
}

void Pool::assignMemory(void* data)
{
	this->mpData = data;
	memset(this->mpData, 0, this->memSize());
}

uIndex Pool::allocate()
{
	assert(this->mpData != nullptr);
	assert(size() < capacity());
	uIndex idx = this->mSize;
	if (this->mEmptySlots.size() > 0)
	{
		auto iter = this->mEmptySlots.begin();
		idx = *iter;
		this->mEmptySlots.erase(iter);
	}
	this->mSize++;
	return idx;
}

void* Pool::at(uIndex idx)
{
	uIndex head = (uSize)this->mpData;
	uIndex atIdx = head + idx * this->mObjectSize;
	void* ptr = (void*)atIdx;
	return ptr;
}

void Pool::deallocate(uIndex idx)
{
	assert(this->mpData != nullptr);
	this->mEmptySlots.insert(idx);
	memset(this->at(idx), 0, this->mObjectSize);
	this->mSize--;
}
