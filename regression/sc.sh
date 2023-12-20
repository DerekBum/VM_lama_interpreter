#!/bin/bash

str=""

for ((i=1;i<=110;i++))
do
    printf -v str "%03d" $i
    #echo $str
    lamac -b test$str.lama
done
