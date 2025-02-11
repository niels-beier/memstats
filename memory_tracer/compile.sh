#!/bin/bash

# Current directory
DIR=$PWD
PINTOOL_PATH=$1

# Remove memorytracer.so if it exists
rm -f $DIR/memorytracer.so

# Copy files to the pintool directory
cp -f $PWD/memorytracer.cc $PINTOOL_PATH/source/tools/ManualExamples/memorytracer.cpp
cp -f $PWD/memorytracer.hh $PINTOOL_PATH/source/tools/ManualExamples/memorytracer.hh

# Compile the memory tracer
cd $PINTOOL_PATH/source/tools/ManualExamples || exit
make obj-intel64/memorytracer.so
cp -f obj-intel64/memorytracer.so $DIR/

# Remove copied files
rm -f obj-intel64/memorytracer.so
rm -f obj-intel64/memorytracer.o
rm -f memorytracer.cpp memorytracer.hh
cd $DIR || exit