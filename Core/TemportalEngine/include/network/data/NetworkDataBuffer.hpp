#pragma once

#include "TemportalEnginePCH.hpp"

#include "utility/Flags.hpp"
#include "network/NetworkCore.hpp"

NS_NETWORK

class Buffer
{
public:
	Buffer(utility::Flags<EType> sourceType, void* pData, uSize size);

	utility::Flags<EType> const& type() const;

	void write(void* data, uSize size);
	void read(void* data, uSize size);

	template <typename T>
	void writeRaw(T const& data)
	{
		this->write((void*)&data, sizeof(T));
	}

	template <typename T>
	void readRaw(T &data)
	{
		this->read(&data, sizeof(T));
	}

	uSize capacity() const;
	uSize size() const;

private:
	utility::Flags<EType> mSourceType;
	void* const mpHead;
	void* mpTail;
	uSize const mCapacity;
	uSize mSize;
};

NS_END
