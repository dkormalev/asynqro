#!/bin/bash

set -e;

THREADS_COUNT=$(( $(cat /proc/cpuinfo | grep processor | wc -l) + 1 ));

BUILD_TYPE=Debug;

if [ "$CXX" = "g++" ]; then
    BUILD_OPTIONS="-fmax-errors=0 -fdiagnostics-color";
else
    BUILD_OPTIONS="-ferror-limit=0 -fcolor-diagnostics";
fi

rm -rf /asynqro/build;
mkdir /asynqro/build;
cd /asynqro/build;
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_BUILD_WITH_GCOV=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
cmake --build . --target all -- -j$THREADS_COUNT;
cmake --build . --target test;
cd /asynqro;
lcov --no-external --capture --directory . --output-file coverage_all.info;
lcov --extract coverage_all.info '*/asynqro/include/*' '*/asynqro/src/*' --output-file coverage.info;
lcov --list coverage.info
