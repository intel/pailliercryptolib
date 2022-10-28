# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#!/bin/bash

if [[ -z "${@}" ]]; then
  echo "Usage: ./reset_asym_buffer_size <old buffer size> <new buffer size>"
  exit
fi

OLD_BUFFER_SIZE=$1

if [[ -z ${2} ]]; then
  echo "Error: second parameter missing"
  echo "Usage: ./reset_asym_buffer_size <old buffer size> <new buffer size>"
  exit 
fi

NEW_BUFFER_SIZE=$2

num_phys_dev=$(lspci -d 8086:4940 | wc -l)
num_virt_dev=$(lspci -d 8086:4941 | wc -l)

# Update physical device configuration files
i=0; 
while [ $i -lt $num_phys_dev ]; 
do 
	sudo sed -i /etc/4xxx_dev$i.conf -e "s/CyNumConcurrentAsymRequests = $OLD_BUFFER_SIZE/CyNumConcurrentAsymRequests = $NEW_BUFFER_SIZE/g"; 
	i=`expr $i + 1`; 
done

# Update virtual function configuration files
i=0; 
while [ $i -lt $num_virt_dev ]; 
do 
	sudo sed -i /etc/4xxxvf_dev$i.conf -e "s/CyNumConcurrentAsymRequests = $OLD_BUFFER_SIZE/CyNumConcurrentAsymRequests = $NEW_BUFFER_SIZE/g"; 
	i=`expr $i + 1`; 
done

## Power Off PFs
#i=0; 
#while [ $i -lt $num_phys_dev ]; 
#do 
#	sudo adf_ctl qat_dev$i down; 
#	i=`expr $i + 1`; 
#done
#
## Power On PFs
#i=0; 
#while [ $i -lt $num_phys_dev ]; 
#do 
#	sudo adf_ctl qat_dev$i up; 
#	i=`expr $i + 1`; 
#done
#
## Restart QAT service (This will bring up PFs and VFs)
#echo "sudo service qat_service restart"
#sudo service qat_service restart
#
## Power Off All VFs
#i=$num_phys_dev;
#vf_per_pf=`expr $num_virt_dev / $num_phys_dev`
#n=`expr $vf_per_pf \\* $num_phys_dev`
#n=`expr $n + $num_phys_dev`
#while [ $i -lt $n ]; 
#do 
#	sudo adf_ctl qat_dev$i down; 
#	i=`expr $i + 1`; 
#done
#
## Power On One QAT VF per QAT PF
#i=$num_phys_dev; 
#while [ $i -lt $n ]; 
#do 
#	sudo adf_ctl qat_dev$i up; 
#	i=`expr $i + $vf_per_pf`; 
#done

