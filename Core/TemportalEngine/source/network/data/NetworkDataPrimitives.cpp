#include "network/data/NetworkDataPrimitives.hpp"

#include "utility/StringUtils.hpp"

#define DEFINE_BUFFER_RAW(DATA_TYPE) \
	void network::write(Buffer &buffer, std::string name, DATA_TYPE value) \
	{ \
		buffer.setNamed(name, utility::StringParser<DATA_TYPE>::to_string(value)); \
		buffer.writeRaw(value); \
	} \
	void network::read(Buffer &buffer, std::string name, DATA_TYPE &value) \
	{ \
		buffer.readRaw(value); \
		buffer.setNamed(name, utility::StringParser<DATA_TYPE>::to_string(value)); \
	}

DEFINE_BUFFER_RAW(bool);
DEFINE_BUFFER_RAW(ui8);
DEFINE_BUFFER_RAW(i8);
DEFINE_BUFFER_RAW(ui16);
DEFINE_BUFFER_RAW(i16);
DEFINE_BUFFER_RAW(ui32);
DEFINE_BUFFER_RAW(i32);
DEFINE_BUFFER_RAW(ui64);
DEFINE_BUFFER_RAW(i64);
DEFINE_BUFFER_RAW(f32);

void network::write(Buffer &buffer, std::string name, std::string value)
{
	buffer.setNamed(name, value);
	uSize const length = value.length();
	buffer.writeRaw(length);
	for (uIndex i = 0; i < length; ++i) buffer.writeRaw(value[i]);
}

void network::read(Buffer &buffer, std::string name, std::string &value)
{
	uSize length;
	buffer.readRaw(length);
	value.resize(length);
	for (uIndex i = 0; i < length; ++i) buffer.readRaw(value[i]);
	buffer.setNamed(name, value);
}

DEFINE_BUFFER_RAW(utility::Guid);
DEFINE_BUFFER_RAW(evcs::EType);
