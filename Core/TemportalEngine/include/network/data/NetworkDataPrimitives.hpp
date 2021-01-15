#pragma once

#include "network/data/NetworkDataBuffer.hpp"

#include "utility/Guid.hpp"

NS_NETWORK

#define DECLARE_BUFFER_OP(DATA_TYPE) \
	void write(Buffer &buffer, DATA_TYPE value); \
	void read(Buffer &buffer, DATA_TYPE &value);

DECLARE_BUFFER_OP(bool);
DECLARE_BUFFER_OP(ui8);
DECLARE_BUFFER_OP(i8);
DECLARE_BUFFER_OP(ui16);
DECLARE_BUFFER_OP(i16);
DECLARE_BUFFER_OP(ui32);
DECLARE_BUFFER_OP(i32);
DECLARE_BUFFER_OP(ui64);
DECLARE_BUFFER_OP(i64);
DECLARE_BUFFER_OP(std::string);

DECLARE_BUFFER_OP(utility::Guid);

template <typename T>
void write(Buffer &buffer, std::vector<T> value)
{
	uSize const length = value.size();
	buffer.writeRaw(length);
	buffer.write((void*)value.data(), length * sizeof(T));
}

template <typename T>
void read(Buffer &buffer, std::vector<T> &value)
{
	uSize length;
	buffer.readRaw(length);
	value.resize(length);
	buffer.read((void*)value.data(), length * sizeof(T));
}

NS_END
