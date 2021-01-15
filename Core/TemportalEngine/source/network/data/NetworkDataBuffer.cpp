#include "network/data/NetworkDataBuffer.hpp"

using namespace network;

Buffer::Buffer(utility::Flags<EType> sourceType, void* pData, uSize size)
	: mSourceType(sourceType)
	, mpHead(pData), mpTail(pData)
	, mCapacity(size), mSize(0)
{
}

utility::Flags<EType> const& Buffer::type() const { return this->mSourceType; }

uSize Buffer::capacity() const { return this->mCapacity; }
uSize Buffer::size() const { return this->mSize; }

void Buffer::write(void* data, uSize size)
{
	if (this->mpTail != nullptr)
	{
		assert(this->mSize + size <= this->mCapacity);
		memcpy_s(this->mpTail, size, data, size);
		this->mpTail = (void*)(uSize(this->mpTail) + size);
	}
	this->mSize += size;
}

void Buffer::read(void* data, uSize size)
{
	if (this->mpTail != nullptr)
	{
		assert(this->mSize + size <= this->mCapacity);
		memcpy_s(data, size, this->mpTail, size);
		this->mpTail = (void*)(uSize(this->mpTail) + size);
	}
	this->mSize += size;
}
