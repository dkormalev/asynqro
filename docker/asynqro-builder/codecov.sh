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
echo -e "\033[1m$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_QT_SUPPORT=ON -DASYNQRO_BUILD_WITH_GCOV=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" .. \033[0m";
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=ON -DASYNQRO_QT_SUPPORT=ON -DASYNQRO_BUILD_WITH_GCOV=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
echo -e "\033[1m$ cmake --build . --target all -- -j$THREADS_COUNT \033[0m";
cmake --build . --target all -- -j$THREADS_COUNT;
cd /asynqro;
echo -e "\033[1m$ lcov --no-external --capture --initial --directory . --output-file code_coverage.baseline\033[0m";
lcov --no-external --capture --initial --directory . --output-file code_coverage.baseline | grep -v 'ignoring data for external file';
echo -e "\033[1m$ lcov --remove code_coverage.baseline '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.baseline\033[0m";
lcov --remove code_coverage.baseline '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.baseline;
cd /asynqro/build;
echo -e "\033[1m$ cmake --build . --target test\033[0m";
cmake --build . --target test || true;
cd /asynqro;
echo -e "\033[1m$ lcov --no-external --capture --directory . --output-file code_coverage.after\033[0m";
lcov --no-external --capture --directory . --output-file code_coverage.after | grep -v 'ignoring data for external file';
echo -e "\033[1m$ lcov --remove code_coverage.after '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.after\033[0m";
lcov --remove code_coverage.after '/usr/*' '/asynqro/tests/*' '/asynqro/build/*' --output-file code_coverage.after;
echo -e "\033[1m$ lcov --add-tracefile code_coverage.baseline --add-tracefile code_coverage.after --output-file code_coverage.total\033[0m";
lcov --add-tracefile code_coverage.baseline --add-tracefile code_coverage.after --output-file code_coverage.total;
echo -e "\033[1m$ lcov --list code_coverage.total\033[0m";
lcov --list code_coverage.total;
echo -e "\033[1m$ lcov -v\033[0m";
lcov -v;
