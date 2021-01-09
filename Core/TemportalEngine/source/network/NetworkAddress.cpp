#include "network/NetworkAddress.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

using namespace network;

Address::Address()
{
	auto steamAddress = SteamNetworkingIPAddr();
	this->mData = *((AddressData*)(&steamAddress));
}

void Address::clear()
{
	reinterpret_cast<SteamNetworkingIPAddr*>(&this->mData)->Clear();
}

bool Address::isEmpty() const
{
	return reinterpret_cast<SteamNetworkingIPAddr const*>(&this->mData)->IsIPv6AllZeros();
}

bool Address::operator==(Address const& other) const
{
	return reinterpret_cast<SteamNetworkingIPAddr const*>(&this->mData) == reinterpret_cast<SteamNetworkingIPAddr const*>(&other.mData);
}

void Address::setIPv6(ui8 const* data, ui16 port)
{
	assert(port >= 0 && port < 65535);
	reinterpret_cast<SteamNetworkingIPAddr*>(&this->mData)->SetIPv6(data, port);
}

void Address::setIPv4(ui32 data, ui16 port)
{
	assert(port >= 0 && port < 65535);
	reinterpret_cast<SteamNetworkingIPAddr*>(&this->mData)->SetIPv4(data, port);
}

void Address::setLocalHost(ui16 port)
{
	assert(port >= 0 && port < 65535);
	reinterpret_cast<SteamNetworkingIPAddr*>(&this->mData)->SetIPv6LocalHost(port);
}

bool Address::isIPv4() const
{
	return reinterpret_cast<SteamNetworkingIPAddr const*>(&this->mData)->IsIPv4();
}

ui32 Address::getIPv4() const
{
	return reinterpret_cast<SteamNetworkingIPAddr const*>(&this->mData)->GetIPv4();
}

bool Address::isLocalHost() const
{
	return reinterpret_cast<SteamNetworkingIPAddr const*>(&this->mData)->IsLocalHost();
}

ui16& Address::port()
{
	return reinterpret_cast<SteamNetworkingIPAddr*>(&this->mData)->m_port;
}

bool Address::fromString(std::string const& str)
{
	return reinterpret_cast<SteamNetworkingIPAddr*>(&this->mData)->ParseString(str.c_str());
}

std::string Address::toString(bool bWithPort) const
{
	auto str = std::string(SteamNetworkingIPAddr::k_cchMaxString, '\0');
	reinterpret_cast<SteamNetworkingIPAddr const*>(&this->mData)->ToString(
		str.data(), str.size(), bWithPort
	);
	return str;
}

void* Address::get() { return &mData; }
