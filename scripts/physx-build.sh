#!/bin/bash

physxConfig=$1
PlatformArchitecture=$2

if [[ "$physxConfig" == "" ]]; then
	echo "No PhysX Configuration. Suggested: \"checked\""
	exit 1
fi
if [[ "$PlatformArchitecture" == "" ]]; then
	echo "No platform architecture. Suggested: \"64\""
	exit 1
fi
echo "Building $physxConfig PhysX on x$PlatformArchitecture"

msbuild="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe"
physxGitModule="Core/TemportalEngine/libs/PhysX"
physxLib="$physxGitModule/physx"
physxGenProjs="$physxLib/generate_projects.sh"
sln="$physxLib/compiler/vc15win64/PhysXSDK.sln"
dlls=(
	"PhysXCommon_$PlatformArchitecture.dll"
	"PhysX_$PlatformArchitecture.dll"
	"PhysXFoundation_$PlatformArchitecture.dll"
	"PhysXCooking_$PlatformArchitecture.dll"
	"PhysXGpu_$PlatformArchitecture.dll"
)
libs=(
	"PhysXCommon_$PlatformArchitecture"
	"PhysX_$PlatformArchitecture"
	"PhysXFoundation_$PlatformArchitecture"
	"PhysXPvdSDK_static_$PlatformArchitecture"
	"PhysXExtensions_static_$PlatformArchitecture"
	"PhysXCharacterKinematic_static_$PlatformArchitecture"
	"PhysXCooking_$PlatformArchitecture"
	"PhysXVehicle_static_$PlatformArchitecture"
	"PhysXTask_static_$PlatformArchitecture"
)

# autoselect the VS17-win<32|64>-v141 option
genProjOption=0
if [[ "$PlatformArchitecture" == "64" ]]; then genProjOption=11; else genProjOption=10; fi
echo "$genProjOption" | $physxGenProjs
physxBin="$physxLib/bin/win.x86_$PlatformArchitecture.vc141.mt/$physxConfig"

# build the configuration
"$msbuild" /property:Configuration=$physxConfig $sln

binaries="Binaries/Build/Debug/x$PlatformArchitecture"

physxLibOut="$binaries/PhysX"
mkdir -p "$physxLibOut"
for libName in "${libs[@]}"
do
	cp "$physxBin/$libName.lib" "$physxLibOut/"
	cp "$physxBin/$libName.pdb" "$physxLibOut/"
done

for dllName in "${dlls[@]}"
do
	cp "$physxBin/$dllName" "$physxLibOut/"
done

cd "$physxGitModule"
git clean -f -d
