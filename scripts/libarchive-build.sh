#!/bin/bash

# ~~~~~ ARGS ~~~~~

PlatformArchitecture="64"
configuration="Debug"

# ~~~~~ CONFIG ~~~~~

name="libarchive"
dir="$name"
sln="ALL_BUILD.vcxproj"
dllPath="bin/$configuration/archive.dll"
pdbPath="bin/$configuration/archive.pdb"
libPath="libarchive/$configuration/archive.lib"

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
