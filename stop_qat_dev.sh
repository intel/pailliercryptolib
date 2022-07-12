#!/bin/env bash

start=$1
stop=$2
step=$3
while [ $start -lt $stop ]
do
  echo "stop qat_dev$start"
  sudo adf_ctl qat_dev$start down 
  start=`expr $start + $step`
done

