#!/bin/bash

set -e;

THREADS_COUNT=$(( $(cat /proc/cpuinfo | grep processor | wc -l) + 1 ));

BUILD_TYPE=Debug;

if [ "$CXX" = "g++" ]; then
    BUILD_OPTIONS="-fmax-errors=0 -fdiagnostics-color";
else
    BUILD_OPTIONS="-ferror-limit=0 -fcolor-diagnostics";
fi


rm -rf /build;
mkdir /build;
cd /build;
echo "$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_BUILD_WITH_GCOV=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" /asynqro"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_BUILD_WITH_GCOV=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" /asynqro;
echo "$ cmake --build . --target all -- -j$THREADS_COUNT"
cmake --build . --target all -- -j$THREADS_COUNT;
echo "$ cmake --build . --target test || true"
cmake --build . --target test || true;
mkdir /asynqro/code_coverage;
cd /asynqro/code_coverage;
gcov -r -s /asynqro `find /build -name "*.gcda"`;
rm main*.gcov *test.h.gcov *_test*.gcov copycountcontainers.h.gcov || true;

# TODO: use lcov again after it will be released with GCC8 support
# lcov --no-external --capture --initial --directory . --output-file code_coverage.baseline;
# lcov --remove code_coverage.baseline '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.baseline;
# cd /asynqro/build;
# cmake --build . --target test || true;
# cd /asynqro;
# lcov --no-external --capture --directory . --output-file code_coverage.after;
# lcov --remove code_coverage.after '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.after;
# lcov --add-tracefile code_coverage.baseline --add-tracefile code_coverage.after --output-file code_coverage.total;
# lcov --list code_coverage.total
