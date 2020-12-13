#!/bin/bash

physxConfig=$1
PlatformArchitecture=$2
projectName=$3

msbuild="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe"
physxGitModule="Core/TemportalEngine/libs/PhysX"
physxLib="$physxGitModule/physx"
physxGenProjs="$physxLib/generate_projects.sh"
sln="$physxLib/compiler/vc15win64/PhysXSDK.sln"
dlls=(
	"PhysXCommon_$PlatformArchitecture.dll" "PhysX_$PlatformArchitecture.dll" "PhysXFoundation_$PlatformArchitecture.dll"
)
libs=(
	"PhysXCommon_$PlatformArchitecture.lib"
	"PhysX_$PlatformArchitecture.lib"
	"PhysXFoundation_$PlatformArchitecture.lib"
	"PhysXPvdSDK_static_$PlatformArchitecture.lib"
	"PhysXExtensions_static_$PlatformArchitecture.lib"
)

# autoselect the VS17-win<32|64>-v141 option
genProjOption=0
if [ $PlatformArchitecture -eq 64 ]; then genProjOption=11; else genProjOption=10; fi
echo "$genProjOption" | $physxGenProjs
physxBin="$physxLib/bin/win.x86_$PlatformArchitecture.vc141.mt/$physxConfig"

# build the configuration
"$msbuild" /property:Configuration=$physxConfig $sln

binaries="Binaries/Build/Debug/x$PlatformArchitecture"

physxLibOut="$binaries/PhysX"
mkdir -p "$physxLibOut"
for libName in "${libs[@]}"
do
	cp "$physxBin/$libName" "$physxLibOut/$libName"
done

projectOut="$binaries/$projectName"
mkdir -p "$projectOut"
for dllName in "${dlls[@]}"
do
	cp "$physxBin/$dllName" "$projectOut/$dllName"
done

cd "$physxGitModule"
git clean -f -d
