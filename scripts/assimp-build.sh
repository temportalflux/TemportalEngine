#!/bin/bash

# ~~~~~ ARGS ~~~~~

PlatformArchitecture=$1
configuration="Debug"

# ~~~~~ CONFIG ~~~~~

name="Assimp"
dir="assimp"
sln="$name.sln"
dllPath="bin/$configuration/assimp-vc141-mtd.dll"
pdbPath="bin/$configuration/assimp-vc141-mtd.pdb"
libPath="lib/$configuration/assimp-vc141-mtd.lib"

# ~~~~~ GENERIC ~~~~~

msbuild="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe"
binaries="$PWD/Binaries/Build/$configuration/x$PlatformArchitecture/$name"

cd "Core/TemportalEngine/libs/$dir"

rm -rf CMakeCache.txt
cmake -G "Visual Studio 15 2017 Win64"
"$msbuild" /property:Configuration=$configuration "$sln"

mkdir -p "$binaries"
cp "$dllPath" "$binaries/"
cp "$pdbPath" "$binaries/"
cp "$libPath" "$binaries/"

git clean -d -f -x
