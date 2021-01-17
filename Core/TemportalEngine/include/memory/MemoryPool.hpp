#pragma once

#include "TemportalEnginePCH.hpp"

NS_MEMORY

class Pool
{

public:

	Pool();
	Pool(uSize objectSize, uSize capacity);
	virtual ~Pool();

	Pool& init(uSize objectSize, uSize capacity);

	uSize capacity() const;
	uSize size() const;
	uSize objectSize() const;
	uSize memSize() const;

	void allocateMemory();
	void assignMemory(void* data);

	uIndex allocate();
	void* at(uIndex idx);
	void deallocate(uIndex idx);

private:
	uSize mObjectSize;
	uSize mCapacity;
	bool mbOwnsMemory;
	void* mpData;
	uSize mSize;
	std::set<uIndex> mEmptySlots;

};

NS_END
