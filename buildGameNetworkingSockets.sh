#!/bin/bash

architecture=$1
target=$2
if [[ "$architecture" == "" ]]; then
	echo "No platform architecture. Suggested: \"64\""
	exit 1
fi
if [[ "$target" == "" ]]; then
	echo "Missing required target binary."
	exit 1
fi
gnsArchitecture="x$architecture-windows"

packageDir="vcpkg"
builtPackage="$packageDir/packages/gamenetworkingsockets_$gnsArchitecture"
binaries="$PWD/Binaries/Build/Debug/x$architecture"
binaryDst="$binaries/GameNetworkingSockets"

echo "Building GameNetworkingSockets on x$architecture for $target"
cd "GNS"

rm -rf "vcpkg"
echo
echo "-----Cloning vcpkg-----"
echo
git clone "git@github.com:microsoft/vcpkg.git" "$packageDir"

echo
echo "----Bootstrapping vcpkg-----"
./$packageDir/bootstrap-vcpkg.sh

echo
echo "-----Compiling GameNetworkingSockets:$gnsArchitecture-----"
echo
./$packageDir/vcpkg.exe --overlay-ports=vcpkg_ports install gamenetworkingsockets --triplet "$gnsArchitecture"

echo
echo "Copying GameNetworkingSockets artifacts"
mkdir -p "$binaryDst"
cp "$builtPackage/debug/bin/GameNetworkingSockets.dll" "$binaries/$target"
cp "$builtPackage/debug/bin/GameNetworkingSockets.pdb" "$binaryDst"
cp "$builtPackage/debug/lib/GameNetworkingSockets.lib" "$binaryDst"
cp "$packageDir/packages/openssl_$gnsArchitecture/debug/bin/libcrypto-1_1-x$architecture.dll" "$binaries/$target"
cp "$packageDir/packages/openssl_$gnsArchitecture/debug/bin/libssl-1_1-x$architecture.dll" "$binaries/$target"
cp "$packageDir/packages/protobuf_$gnsArchitecture/debug/bin/libprotobufd.dll" "$binaries/$target"

rm -rf "vcpkg"