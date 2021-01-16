#pragma once

#include "TemportalEnginePCH.hpp"

#include "utility/Flags.hpp"
#include "network/NetworkCore.hpp"

NS_NETWORK

class Buffer
{
public:
	using StringifiedData = std::vector<std::pair<std::string, std::string>>;

	Buffer(utility::Flags<EType> sourceType, void* pData, uSize size, bool bPopulateStrings);

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

	void setNamed(std::string name, std::string value);
	void stringify(std::stringstream &ss) const;

	uSize capacity() const;
	uSize size() const;

private:
	utility::Flags<EType> mSourceType;
	void* const mpHead;
	void* mpTail;
	uSize const mCapacity;
	uSize mSize;
	bool mbPopulateStrings;
	StringifiedData mStringifiedData;
};

NS_END
