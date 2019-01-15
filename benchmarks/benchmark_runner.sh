#!/bin/bash

for i in {0..4}; do
    for benchmark in bins/*; do
        name=`basename $benchmark`;
        mkdir -p results/$name;
        echo "$name: attempt#$i";
        $benchmark > results/$name/$i 2> results/$name/${i}_err || true;
    done
done
