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

echo "$ rm -rf /usr/include/x86_64-linux-gnu/qt5 || true"
rm -rf /usr/include/x86_64-linux-gnu/qt5 || true;
echo "$ rm -rf /usr/lib/x86_64-linux-gnu/cmake/Qt5* || true"
rm -rf /usr/lib/x86_64-linux-gnu/cmake/Qt5* || true;
echo "$ rm -rf /usr/lib/x86_64-linux-gnu/libQt5* || true"
rm -rf /usr/lib/x86_64-linux-gnu/libQt5* || true;
echo "$ rm -rf /usr/lib/x86_64-linux-gnu/pkgconfig/Qt5* || true"
rm -rf /usr/lib/x86_64-linux-gnu/pkgconfig/Qt5* || true;

rm -rf /asynqro/build;
mkdir /asynqro/build;
cd /asynqro/build;
echo "$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" .."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
echo "$ cmake --build . --target all -- -j$THREADS_COUNT"
cmake --build . --target all -- -j$THREADS_COUNT;
echo "$ cmake --build . --target test"
cmake --build . --target test
