#!/bin/bash

cd "GNS"

rm -rf "vcpkg"
echo
echo "-----Cloning vcpkg-----"
echo
git clone "git@github.com:microsoft/vcpkg.git" "$packageDir"

echo
echo "----Bootstrapping vcpkg-----"
./vcpkg/bootstrap-vcpkg.sh
