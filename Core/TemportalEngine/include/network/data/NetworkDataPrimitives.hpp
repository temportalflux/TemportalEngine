#pragma once

#include "network/data/NetworkDataBuffer.hpp"

#include "utility/Guid.hpp"

NS_NETWORK

#define DECLARE_BUFFER_OP(DATA_TYPE) \
	void write(Buffer &buffer, std::string name, DATA_TYPE value); \
	void read(Buffer &buffer, std::string name, DATA_TYPE &value);

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
void write(Buffer &buffer, std::string name, std::vector<T> value)
{
	uSize const length = value.size();
	buffer.setNamed(name, std::to_string(length * sizeof(T)) + " bytes");
	buffer.writeRaw(length);
	buffer.write((void*)value.data(), length * sizeof(T));
}

template <typename T>
void read(Buffer &buffer, std::string name, std::vector<T> &value)
{
	uSize length;
	buffer.readRaw(length);
	value.resize(length);
	buffer.read((void*)value.data(), length * sizeof(T));
	buffer.setNamed(name, std::to_string(length * sizeof(T)) + " bytes");
}

template <typename T, uSize S>
void write(Buffer &buffer, std::string name, math::Vector<T, S> value)
{
	buffer.setNamed(name, value.toString());
	buffer.write(value.data(), S * sizeof(T));
}

template <typename T, uSize S>
void read(Buffer &buffer, std::string name, math::Vector<T, S> &value)
{
	buffer.read(value.data(), S * sizeof(T));
	buffer.setNamed(name, value.toString());
}

NS_END
