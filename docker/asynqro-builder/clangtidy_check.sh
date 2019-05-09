#!/bin/bash

set -e;

THREADS_COUNT=$(( $(cat /proc/cpuinfo | grep processor | wc -l) + 1 ));

BUILD_TYPE=Debug;
BUILD_OPTIONS="-g -O0 -fno-inline -ferror-limit=0 -fcolor-diagnostics -stdlib=libc++ -isystem /usr/lib/llvm-8/include/c++/v1";
CLANG_TIDY_CHECKS="-*,clang-analyzer-*,bugprone-*,cert-dcl21-cpp,cert-dcl50-cpp,cert-dcl58-cpp,cert-env33-c,cert-err34-c,cert-err52-cpp,cert-err60-cpp,cert-flp30-c,cert-msc50-cpp,cert-msc51-cpp,cppcoreguidelines-avoid-goto,cppcoreguidelines-c-copy-assignment-signature,cppcoreguidelines-interfaces-global-init,cppcoreguidelines-no-malloc,cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-type-member-init,cppcoreguidelines-pro-type-union-access,cppcoreguidelines-slicing,cppcoreguidelines-special-member-functions,performance-*,google-build-explicit-make-pair,google-build-namespaces,google-explicit-constructor,google-readability-namespace-comments,google-runtime-operator,hicpp-exception-baseclass,hicpp-multiway-paths-covered,hicpp-no-assembler,hicpp-noexcept-move,hicpp-signed-bitwise,llvm-include-order,llvm-twine-local,misc-new-delete-overloads,misc-non-copyable-objects,misc-static-assert,misc-throw-by-value-catch-by-reference,modernize-avoid-bind,modernize-deprecated-headers,modernize-loop-convert,modernize-make-*,modernize-raw-string-literal,modernize-redundant-void-arg,modernize-replace-*,modernize-shrink-to-fit,modernize-unary-static-assert,modernize-use-bool-literals,modernize-use-default-member-init,modernize-use-emplace,modernize-use-equals-delete,modernize-use-noexcept,modernize-use-nullptr,modernize-use-transparent-functors,modernize-use-uncaught-exceptions,modernize-use-using,readability-avoid-const-params-in-decls,readability-container-size-empty,readability-delete-null-pointer,readability-deleted-default,readability-else-after-return,readability-function-size,readability-identifier-naming,readability-inconsistent-declaration-parameter-name,readability-misleading-indentation,readability-misplaced-array-index,readability-non-const-parameter,readability-redundant-control-flow,readability-redundant-declaration,readability-redundant-function-ptr-dereference,readability-redundant-smartptr-get,readability-redundant-string-*,readability-simplify-*,readability-static-*,readability-string-compare,readability-uniqueptr-delete-release,readability-rary-objects";
ERRORS_FILE=/clangtidy_errors.log;

rm -rf /asynqro/build;
mkdir /asynqro/build;
cd /asynqro/build;
echo -e "\033[1m$ cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=OFF -DASYNQRO_QT_SUPPORT=ON -DASYNQRO_BUILD_WITH_DUMMY=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \"-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS\" ..\033[0m"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DASYNQRO_BUILD_TESTS=OFF -DASYNQRO_QT_SUPPORT=ON -DASYNQRO_BUILD_WITH_DUMMY=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "-DCMAKE_CXX_FLAGS=$BUILD_OPTIONS" ..;
echo -e "\033[1m$ /run-clang-tidy-asynqro.py -header-filter='.*(h|cpp)$' -checks=\"$CLANG_TIDY_CHECKS\" -j$THREADS_COUNT -quiet\033[0m";
/run-clang-tidy-asynqro.py  -header-filter='.*(h|cpp)$' -checks="$CLANG_TIDY_CHECKS" -j$THREADS_COUNT -quiet > $ERRORS_FILE;

FOUND=1;
if [ ! -f "$ERRORS_FILE" ]; then
    FOUND=0;
elif [ `grep -sc -v '^ *$' "$ERRORS_FILE"` -eq 0 ]; then
    FOUND=0;
fi

if [ $FOUND -ne 0 ]; then
    echo -e "\033[1mClang-tidy found next issues:\033[0m";
    cat $ERRORS_FILE;
fi

exit $FOUND;
