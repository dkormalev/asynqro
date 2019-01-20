#!/bin/bash

set -e;

THREADS_COUNT=$(( $(cat /proc/cpuinfo | grep processor | wc -l) + 1 ));

if [ -n "$1" ]; then
    BUILD_TYPE=$1;
else
    BUILD_TYPE=Release;
fi

if [ "$CXX" = "g++" ]; then
    BUILD_OPTIONS="-fmax-errors=0 -fdiagnostics-color"
else
    BUILD_OPTIONS="-ferror-limit=0 -fcolor-diagnostics"
fi

rm -rf /asynqro/build;
mkdir /asynqro/build;
cd /asynqro/build;
echo "$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" .."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
echo "$ cmake --build . --target all -- -j$THREADS_COUNT"
cmake --build . --target all -- -j$THREADS_COUNT;
echo "$ cmake --build . --target test"
cmake --build . --target test
