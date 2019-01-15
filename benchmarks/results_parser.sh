#!/bin/bash

empty_repost_name_parser="^(.*)_c([0-9]+)_j([0-9]+)_empty_repost"
timed_repost_name_parser="^(.*)_c([0-9]+)_j([0-9]+)_l([0-9]+)_timed_repost"
empty_avalanche_name_parser="^(.*)_j([0-9]+)_empty_avalanche"
timed_avalanche_name_parser="^(.*)_j([0-9]+)_l([0-9]+)_timed_avalanche"

rm -rf parsed_results

for f in results/*; do
    name=`basename $f`
    if [[ $name =~ $empty_repost_name_parser ]]; then
        system=${BASH_REMATCH[1]}
        concurrency=${BASH_REMATCH[2]}
        jobs=${BASH_REMATCH[3]}
        echo "empty_repost $system with c=$concurrency and j=$jobs"
        mkdir -p parsed_results/empty_repost/c${concurrency}_j${jobs}
        for try in $f/*; do
            grep "reposted" $try | sed -E 's|reposted [0-9]* in ([0-9.e+\-]*) ms|\1|' | sort -nr | head -1 >> parsed_results/empty_repost/c${concurrency}_j${jobs}/${system}
        done
    elif [[ $name =~ $timed_repost_name_parser ]]; then
        system=${BASH_REMATCH[1]}
        concurrency=${BASH_REMATCH[2]}
        jobs=${BASH_REMATCH[3]}
        length=${BASH_REMATCH[4]}
        echo "timed_repost $system with c=$concurrency and j=$jobs"
        mkdir -p parsed_results/timed_repost/c${concurrency}_l${length}_j${jobs}
        for try in $f/*; do
            grep "adjusted useful" $try | sed -E 's|total ([0-9.e+\-]*) ms; useful ([0-9.e+\-]*) ms; adjusted useful ([0-9.e+\-]*) ms; overhead ([0-9.e+\-]*) ms|\4,\1,\2|' >> parsed_results/timed_repost/c${concurrency}_l${length}_j${jobs}/${system}
        done
    elif [[ $name =~ $empty_avalanche_name_parser ]]; then
        system=${BASH_REMATCH[1]}
        jobs=${BASH_REMATCH[2]}
        echo "empty_avalanche $system with c=$concurrency and j=$jobs"
        mkdir -p parsed_results/empty_avalanche/j$jobs
        for try in $f/*; do
            grep "processed" $try | sed -E 's|processed [0-9]* in ([0-9.e+\-]*) ms|\1|' >> parsed_results/empty_avalanche/j$jobs/${system}
        done
    elif [[ $name =~ $timed_avalanche_name_parser ]]; then
        system=${BASH_REMATCH[1]}
        jobs=${BASH_REMATCH[2]}
        length=${BASH_REMATCH[3]}
        echo "timed_avalanche $system with c=$concurrency and j=$jobs"
        mkdir -p parsed_results/timed_avalanche/l${length}_j${jobs}
        for try in $f/*; do
            grep "adjusted useful" $try | sed -E 's|total ([0-9.e+\-]*) ms; useful ([0-9.e+\-]*) ms; adjusted useful ([0-9.e+\-]*) ms; overhead ([0-9.e+\-]*) ms|\4,\1,\2|' >> parsed_results/timed_avalanche/l${length}_j${jobs}/${system}
        done
    else
        echo "$name not recognized!"
        continue
    fi
done
