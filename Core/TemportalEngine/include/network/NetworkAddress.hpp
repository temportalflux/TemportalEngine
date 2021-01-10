#pragma once

#include "TemportalEnginePCH.hpp"

NS_NETWORK

// Mirror of SteamNetworkingIPAddr
class Address
{

public:
	Address();

	void clear();
	bool isEmpty() const;
	bool operator==(Address const& other) const;

	void setIPv6(ui8 const* data, ui16 port);
	void setIPv4(ui32 data, ui16 port);
	Address& setLocalHost(ui16 port);
	Address& setPort(ui16 port);

	bool isIPv4() const;
	ui32 getIPv4() const;
	ui16& port();
	bool isLocalHost() const;

	bool fromString(std::string const& str);
	std::string toString(bool bWithPort) const;

	void* get();

private:
#pragma pack(push,1)
	struct AddressData
	{
		ui8 mIPv6[16];
		ui16 mPort;
	};
#pragma pack(pop)
	AddressData mData;

};

NS_END
