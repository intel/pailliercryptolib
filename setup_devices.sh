#!/bin/bash

num_phys_dev=$(lspci -d 8086:4940 | wc -l) 
if [ $num_phys_dev -eq 0 ]; then
  echo "No QAT Device Found !"
  exit
else
  echo "$num_phys_dev QAT Devices Found !"	
fi

total_virt_func=$(lspci -d 8086:4941 | wc -l) 
num_virt_func=`expr $total_virt_func / $num_phys_dev`
dev_step=1

if [ $# -eq 0 ]; then
  echo "Usage: ./setup_devices <num_phys_dev> <num_virt_inst> <conf_virt_inst>"
  echo "   Parameters:"
  echo "   -- num_phys_dev:   Number of physical devices to be active. (default: auto)"
  echo "   -- conf_virt_func: Number of configured virtual functions per device. (default: 0)"
  echo "   -- num_virt_func:  Number of virtual functions to be active per device. (default: 0)"
fi

nphysdev=$num_phys_dev
if [ -n "$1" ]; then
  nphysdev=$1
  if [ $nphysdev -gt $num_phys_dev ]; then
    nphysdev=$num_phys_dev
  fi
fi

conf_virt_func=0
# Check if virtual function is enabled
if [ $num_virt_func -gt 0 ]; then
  conf_virt_func=1
fi 

if [ -n "$2" ]; then
  conf_virt_func=$2
  # if user attempts to request higher than available
  if [ $conf_virt_func -gt $num_virt_func ]; then
    conf_virt_func=$num_virt_func
  fi
fi

start=0
# If on virtualization mode
if [ $num_virt_func -gt 0 & $conf_virt_func -gt 0 ]; then 
  start=$num_phys_dev
  dev_step=$num_virt_func
fi

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
  while [ $j -lt $conf_virt_func ]; 
  do
    dev_id=`expr $j + $start`;
    sudo adf_ctl qat_dev$dev_id up;
    j=`expr $j + 1`;
  done
  start=`expr $start + $dev_step`
done


