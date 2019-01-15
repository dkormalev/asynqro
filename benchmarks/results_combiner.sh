#!/bin/bash

rm -f parsed_results/*.csv

for f in parsed_results/empty_avalanche/*; do
    name=`basename $f`
    if [[ $name =~ ^j([0-9]+)$ ]]; then
        jobs=${BASH_REMATCH[1]}
        filename="parsed_results/empty_avalanche_chart.csv"
        for s in $f/*; do
            system=`basename $s`
            best=`sort -n $s | head -1`
            if [ ! -f $filename ]; then
                echo "system,jobs,total" > $filename
            fi
            echo "$system,$jobs,$best" >> $filename
        done
    else
        echo "Can't parse $name"
    fi
done

for f in parsed_results/empty_repost/*; do
    name=`basename $f`
    if [[ $name =~ ^c([0-9]+)_j([0-9]+)$ ]]; then
        concurrency=${BASH_REMATCH[1]}
        jobs=${BASH_REMATCH[2]}
        filename="parsed_results/empty_repost_j${jobs}_chart.csv"
        for s in $f/*; do
            system=`basename $s`
            best=`sort -n $s | head -1`
            if [ ! -f $filename ]; then
                echo "system,concurrency,total" > $filename
            fi
            echo "$system,$concurrency,$best" >> $filename
        done
    else
        echo "Can't parse $name"
    fi
done

for f in parsed_results/timed_avalanche/*; do
    name=`basename $f`
    if [[ $name =~ ^l([0-9]+)_j([0-9]+)$ ]]; then
        length=${BASH_REMATCH[1]}
        jobs=${BASH_REMATCH[2]}
        filename="parsed_results/timed_avalanche_l${length}_chart.csv"
        for s in $f/*; do
            system=`basename $s`
            best=`sort -n $s | head -1`
            if [ ! -f $filename ]; then
                echo "system,jobs,overhead,total,payload" > $filename
            fi
            echo "$system,$jobs,$best" >> $filename
        done
    else
        echo "Can't parse $name"
    fi
done

for f in parsed_results/timed_repost/*; do
    name=`basename $f`
    if [[ $name =~ ^c([0-9]+)_l([0-9]+)_j([0-9]+)$ ]]; then
        concurrency=${BASH_REMATCH[1]}
        length=${BASH_REMATCH[2]}
        jobs=${BASH_REMATCH[3]}
        filename="parsed_results/timed_repost_l${length}_j${jobs}_chart.csv"
        for s in $f/*; do
            system=`basename $s`
            best=`sort -n $s | head -1`
            if [ ! -f $filename ]; then
                echo "system,concurrency,overhead,total,payload" > $filename
            fi
            echo "$system,$concurrency,$best" >> $filename
        done
    else
        echo "Can't parse $name"
    fi
done
