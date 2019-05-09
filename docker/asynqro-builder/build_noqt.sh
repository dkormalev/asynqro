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

echo -e "\033[1m$ rm -rf /usr/include/x86_64-linux-gnu/qt5 || true\033[0m"
rm -rf /usr/include/x86_64-linux-gnu/qt5 || true;
echo -e "\033[1m$ rm -rf /usr/lib/x86_64-linux-gnu/cmake/Qt5* || true\033[0m"
rm -rf /usr/lib/x86_64-linux-gnu/cmake/Qt5* || true;
echo -e "\033[1m$ rm -rf /usr/lib/x86_64-linux-gnu/libQt5* || true\033[0m"
rm -rf /usr/lib/x86_64-linux-gnu/libQt5* || true;
echo -e "\033[1m$ rm -rf /usr/lib/x86_64-linux-gnu/pkgconfig/Qt5* || true\033[0m"
rm -rf /usr/lib/x86_64-linux-gnu/pkgconfig/Qt5* || true;

rm -rf /asynqro/build;
mkdir /asynqro/build;
cd /asynqro/build;
echo -e "\033[1m$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" ..\033[0m"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
echo -e "\033[1m$ cmake --build . --target all -- -j$THREADS_COUNT\033[0m"
cmake --build . --target all -- -j$THREADS_COUNT;
echo -e "\033[1m$ cmake --build . --target test\033[0m"
cmake --build . --target test
