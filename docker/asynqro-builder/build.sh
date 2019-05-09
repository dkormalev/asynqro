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
echo -e "\033[1m$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_QT_SUPPORT=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" ..\033[0m"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_QT_SUPPORT=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
echo -e "\033[1m$ cmake --build . --target all -- -j$THREADS_COUNT\033[0m"
cmake --build . --target all -- -j$THREADS_COUNT;
echo -e "\033[1m$ cmake --build . --target test\033[0m"
cmake --build . --target test
