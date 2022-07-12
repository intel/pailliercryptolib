#! /bin/bash

# Command Line Parameters
#   - 1 => Enable Telemetry
#   - 0 => Disable Telemetry
if [ $# == 0 ];
then
        echo "telemetry helper usage:"
        echo " ./telemetry_helper.sh 1    <= Enables Telemetry service on all QAT end points"
        echo " ./telemetry_helper.sh 0    <= Disables Telemetry service on all QAT end points"
        exit
fi

# ensure cr/lf are not removed from lspci command
IFS=

# Capture all QuickAssist Device id info to logfile
echo "$(lspci -d 8086:4940)" > pci_ids.txt
control_file_names=()

# Parse the logfile extracting just the pci device ids to array
while IFS= read -r line; do
        bus_num=${line:0:2}
        control_file_name="/sys/devices/pci0000:"$bus_num"/0000:"$bus_num":00.0/telemetry/control"
        control_file_names+=($control_file_name)
        done < pci_ids.txt

        for ((i=0; i<${#control_file_names[@]}; i++))
        do
                if [ $1 = 0 ];
                then
                        echo "Disabling telemetry for " ${control_file_names[$i]}
                else
                        echo "Enabling telemetry for " ${control_file_names[$i]}
                fi
                echo $1 > ${control_file_names[$i]}
        done

# Remove temporary file
rm pci_ids.txt

