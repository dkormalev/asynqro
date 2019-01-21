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
echo "$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_BUILD_WITH_GCOV=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" .."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_BUILD_WITH_GCOV=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
echo "$ cmake --build . --target all -- -j$THREADS_COUNT"
cmake --build . --target all -- -j$THREADS_COUNT;
cd /asynqro;
lcov --no-external --capture --initial --directory . --output-file code_coverage.baseline;
lcov --remove code_coverage.baseline '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.baseline;
cd /asynqro/build;
echo "$ cmake --build . --target test || true"
cmake --build . --target test || true;
cd /asynqro;
lcov --no-external --capture --directory . --output-file code_coverage.after;
lcov --remove code_coverage.after '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.after;
lcov --add-tracefile code_coverage.baseline --add-tracefile code_coverage.after --output-file code_coverage.total;
lcov --list code_coverage.total;
lcov -v;
