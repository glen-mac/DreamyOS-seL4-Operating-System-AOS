#!/bin/sh
# @LICENSE(NICTA)

#This is a quick a dirty script I wrote to rebuild all the prebuilt muslc library files
#To run this script first modify the muslc makefile to not reference the prebuilt libraries
#Ensure all the right toolchains exist on your path or wherever the config files expect them to be
#Then execute this script from the root directory of sel4test (or any project with all the
#right configuration files.
#when it's done revert the Makefile and commit the new libraries
#Under the assumption that it fails, don't blame me

for config in `ls configs/*_debug_xml_defconfig | sed 's/configs\///'`
do
    plat=`grep "^CONFIG_PLAT_" "configs/$config" | sed 's/.*_\([A-Z0-9]*\).*/\1/' | tr 'A-Z' 'a-z' | sed 's/kzm/imx31/'`
    arch=`grep "^CONFIG_ARCH_[A-Z0-9]*=" "configs/$config" | grep -v "I386" | sed 's/.*_\([A-Z0-9]*\).*/\1/' | tr 'A-Z' 'a-z'`
    make clean
    make $config
    make silentoldconfig
    make libmuslc
    cp "build/$arch/$plat/libmuslc/libmuslc.a" "libs/libmuslc/pre-built/libmuslc-$arch-$plat.a"
done
