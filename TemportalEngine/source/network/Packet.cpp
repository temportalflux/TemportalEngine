#include "network/Packet.hpp"

using namespace network;

/*
Packet::Packet(const unsigned int lengthAddress, const char* address, const unsigned int dataLength, const unsigned char* data)
{
	mAddressLength = lengthAddress;
	mAddress = address;
	mDataLength = dataLength;
	// Copy the data, thereby creating a new pointer array
	copy(data, mData, dataLength);
}

Packet::~Packet()
{
	mAddressLength = 0;
	mDataLength = 0;
	mAddress = NULL;
	// Data is assumed to have been copied, and in doing so, created a new pointer
	delete[] mData;
	mData = NULL;
}

// Create a new char[] at dest, and copies the contents of the range [0, length) from source into dest
void Packet::copy(const unsigned char* &source, unsigned char* &dest, unsigned int length)
{
	// Allocate the memory for the data copy
	dest = new unsigned char[length];
	// Linearly copy all data from source into dest from 0 to length
	for (unsigned int i = 0; i < length; i++)
	{
		dest[i] = source[i];
	}
}

// Sets address and length to the address characters of the ip address
void Packet::getAddress(const char* &address, unsigned int &length)
{
	address = this->mAddress;
	length = this->mAddressLength;
}

std::string Packet::getAddress()
{
	const char *address;
	unsigned int length;
	this->getAddress(address, length);
	return std::string(address);
}

// Sets data and length to the byte data from the packet
void Packet::getData(unsigned char* &data, unsigned int &length)
{
	data = this->mData;
	length = this->mDataLength;
}
//*/