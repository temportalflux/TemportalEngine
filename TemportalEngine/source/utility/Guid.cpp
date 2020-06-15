#include "utility/Guid.hpp"

#ifdef _WIN32
#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include "Windows.h"
#include "Rpc.h"
#endif

using namespace utility;

#define GUID_STRING_LENGTH 36

Guid Guid::create()
{
#ifdef _WIN32
	UUID uuid;
	UuidCreate(&uuid);
	return Guid(*((Data*)(&uuid)));
#else
	assert(false);
	return Guid();
#endif
}

Guid Guid::fromString(std::string const str)
{
#ifdef _WIN32
	UUID uuid;
	UuidFromString((RPC_CSTR)str.data(), &uuid);
	return Guid(*((Data*)(&uuid)));
#else
	assert(false);
	return Guid();
#endif
}

i32 Guid::compare(Guid const &a, Guid const &b)
{
#ifdef _WIN32
	UUID* uuidA = (UUID*)(&a.mData);
	UUID* uuidB = (UUID*)(&b.mData);
	RPC_STATUS errors = 0;
	return UuidCompare(uuidA, uuidB, &errors);
#else
	assert(false);
	return 0;
#endif
}

Guid::Guid() : mData({})
{
}

Guid::Guid(Guid const &other) : mData(other.mData)
{
}

Guid::Guid(Data const &data) : mData(data)
{
}

bool Guid::isValid() const
{
	return *this != Guid();
}

std::string Guid::toString() const
{
	std::string str = "";
#ifdef _WIN32
	RPC_CSTR pStr;
	UuidToString((UUID*)(&mData), &pStr);
	str.assign((char*)pStr);
	RpcStringFree(&pStr);
	return str;
#else
	assert(false);
#endif
	return str;
}

bool Guid::operator<(Guid const &other) const
{
	return Guid::compare(*this, other) < 0;
}

bool Guid::operator>(Guid const &other) const
{
	return Guid::compare(*this, other) > 0;
}

bool Guid::operator==(Guid const &other) const
{
	return Guid::compare(*this, other) == 0;
}

bool Guid::operator!=(Guid const &other) const
{
	return Guid::compare(*this, other) != 0;
}

void Guid::operator=(Guid const &other)
{
	memcpy_s((void*)&mData, sizeof(Data), (void*)&other.mData, sizeof(Data));
}

ui32 Guid::hash() const
{
#ifdef _WIN32
	UUID* uuid = (UUID*)(&mData);
	RPC_STATUS errors;
	return UuidHash(uuid, &errors);
#else
	assert(false);
	return 0;
#endif
}
