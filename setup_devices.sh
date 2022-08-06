#!/bin/env bash

num_phys_dev=8   # num_dev=$(lspci -d 8086:4940 | wc -l)
conf_virt_func=0 # total_virt_func=$(lspci -d 8086:4941 | wc -l)
num_virt_func=0  # conf_virt_func=`expr $total_virt_func / $num_dev`
dev_step=1

if [ $# -eq 0 ]; then
  echo "Parameters:"
  echo "-- num_phys_dev: Number of physical devices to be active. (default: 8)"
  echo "-- conf_virt_func: Number of configured virtual functions per device. (default: 0)"
  echo "-- num_virt_func: Number of virtual functions to be active per device. (default: 0)"
  echo "./setup_devices <num_phys_dev> <num_virt_inst> <conf_virt_inst>"
fi

if [ -n "$1" ]; then
  num_phys_dev=$1
fi

if [ -n "$2" ]; then
  conf_virt_func=$2
fi

if [ -n "$3" ]; then
  num_virt_func=$3
fi

if [ $conf_virt_func -gt 0 ]; then 
  dev_step=$conf_virt_func
fi

start=$num_phys_dev
stop=`expr $num_phy_dev \\* $conf_virt_func`
stop=`expr $start + $stop`
step=$dev_step

i=$start; 
n=`expr $start \\* $conf_virt_func`; 
n=`expr $i + $n`; 
while [ $i -lt $n ]; 
do 
  echo "sudo adf_ctl qat_dev$i down"; 
  sudo adf_ctl qat_dev$i down; 
  i=`expr $i + 1`; 
done

while [ $start -lt $stop ];
do
  echo "adf_ctl qat_dev$start up"
  sudo adf_ctl qat_dev$start up;
  # start up additional instances mapped to the same physical device
  j=1;
  while [ $j -lt $num_virt_func ]; 
  do
    dev_id=`expr $j + $start`;
    sudo adf_ctl qat_dev$dev_id up;
    j=`expr $j + 1`;
  done
  start=`expr $start + $dev_step`
done


