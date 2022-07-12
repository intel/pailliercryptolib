#!/bin/env bash

start=$1
stop=$2
step=$3
while [ $start -lt $stop ]
do
  echo "start qat_dev$start"
  sudo adf_ctl qat_dev$start up 
  start=`expr $start + $step`
done

