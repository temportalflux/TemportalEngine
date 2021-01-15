#!/bin/bash

packageDir="vcpkg"
root="$PWD"

cd "GNS"

function compileGNS()
{
	architecture=$1
	gnsArchitecture="x$architecture-windows"
	
	echo
	echo "-----Compiling GameNetworkingSockets:$gnsArchitecture-----"
	echo
	./$packageDir/vcpkg.exe --overlay-ports=vcpkg_ports install gamenetworkingsockets --triplet "$gnsArchitecture"

	builtPackage="$packageDir/packages/gamenetworkingsockets_$gnsArchitecture"
	binaries="$root/Binaries/Build/Debug/x$architecture"

	echo
	echo "Copying GameNetworkingSockets artifacts"

	binaryDst="$binaries/GameNetworkingSockets"
	mkdir -p "$binaryDst"
	cp "$builtPackage/debug/bin/GameNetworkingSockets.pdb" "$binaryDst"
	cp "$builtPackage/debug/lib/GameNetworkingSockets.lib" "$binaryDst"
	cp "$builtPackage/debug/bin/GameNetworkingSockets.dll" "$binaryDst"

	binaryDst="$binaries/OpenSSL"
	mkdir -p "$binaryDst"
	cp "$packageDir/packages/openssl_$gnsArchitecture/lib/libssl.lib" "$binaryDst"
	cp "$packageDir/packages/openssl_$gnsArchitecture/lib/libcrypto.lib" "$binaryDst"
	cp "$packageDir/packages/openssl_$gnsArchitecture/debug/bin/libcrypto-1_1-x$architecture.dll" "$binaryDst"
	cp "$packageDir/packages/openssl_$gnsArchitecture/debug/bin/libssl-1_1-x$architecture.dll" "$binaryDst"
	cp "$packageDir/packages/protobuf_$gnsArchitecture/debug/bin/libprotobufd.dll" "$binaryDst"

}

#compileGNS 86
compileGNS 64

echo
echo "Copying OpenSSL include"
opensslInclude="$root/Core/TemportalEngine/libs/OpenSSL"
mkdir -p "$opensslInclude"
cp -r "$packageDir/packages/openssl_x64-windows/include/" "$opensslInclude"
