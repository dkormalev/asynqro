#!/bin/bash

set -e;

cd /asynqro;
echo -e "\033[1m$ find -iname '*.h' -o -iname '*.cpp' | xargs clang-format-6.0 -i\033[0m";
find -iname '*.h' -o -iname '*.cpp' | xargs clang-format-6.0 -i;
echo -e "\033[1m$ git diff --stat:\033[0m";
git diff --stat;
echo -e "\033[1m$ git diff:\033[0m";
git diff;

exit `git diff | wc -l`
