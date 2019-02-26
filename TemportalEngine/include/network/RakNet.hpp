#ifndef NETWORK_RAKNET_HPP
#define NETWORK_RAKNET_HPP

// work around for known issue
// https://developercommunity.visualstudio.com/content/problem/185399/error-c2760-in-combaseapih-with-windows-sdk-81-and.html
struct IUnknown;

#include <RakNet/WindowsIncludes.h>
#include <Windows.h>
#include <RakNet\RakNetTime.h>
#include <RakNet\RakPeerInterface.h>
#include <RakNet\MessageIdentifiers.h>
#include <RakNet\GetTime.h>

#endif