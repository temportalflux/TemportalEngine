#!/bin/bash

architecture=$1
target=$2

root=$PWD
vcpkgRemote="git@github.com:microsoft/vcpkg.git"
EngineLibs="Core/TemportalEngine/libs"
gnsSrc="$EngineLibs/GameNetworkingSockets"
packageDir="$EngineLibs/GameNetworkingSockets-package"
vcpkgExe="$packageDir/vcpkg.exe"
gnsArchitecture="x86-windows" # x64 GNS has compilation errors in VCPKG
builtPackage="$packageDir/packages/gamenetworkingsockets_$gnsArchitecture"
binaries="$root/Binaries/Build/Debug/x$architecture"
binaryDst="$binaries/GameNetworkingSockets"

# Initialize vcpkg for GameNetworkingSockets
rm -rf "$packageDir"
git clone "$vcpkgRemote" "$packageDir"
cd "$root/$packageDir"
./bootstrap-vcpkg.sh

# Compile GameNetworkingSockets
cd "$root/$gnsSrc"
"$root/$vcpkgExe" --overlay-ports=vcpkg_ports install gamenetworkingsockets --triplet "$gnsArchitecture"

# Copy artifacts to binaries
mkdir -p "$binaryDst"
cp "$root/$builtPackage/debug/bin/GameNetworkingSockets.dll" "$binaries/$target"
cp "$root/$builtPackage/debug/bin/GameNetworkingSockets.pdb" "$binaryDst"
cp "$root/$builtPackage/debug/lib/GameNetworkingSockets.lib" "$binaryDst"
