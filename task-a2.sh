#!/bin/bash

file_name="scratch/task_A2.cc"

_vary_nodes(){
    nodes=(20 40 60 80 100)

    for (( i = 0; i < 5; i++ ))
    do 
        
        first_data=1
        if [ $i -gt 0 ];
        then
            first_data=0
        fi
        echo nodes : ${nodes[i]} : $first_data
        # echo "$file_name --first_data=${first_data} --number_of_nodes=${nodes[i]}" 
        ./waf --run  "$file_name --changing_parameter=node --first_data=${first_data} --number_of_nodes=${nodes[i]}" 

    done
}

_vary_flows(){
    nodes=(10 20 30 40 50)

    for (( i = 0; i < 5; i++ ))
    do 
        
        first_data=1
        if [ $i -gt 0 ];
        then
            first_data=0
        fi
        echo flows : ${nodes[i]} : $first_data
        # echo "$file_name --first_data=${first_data} --number_of_nodes=${nodes[i]}" 
        ./waf --run  "$file_name --changing_parameter=flow --first_data=${first_data} --number_of_flows=${nodes[i]}" 

    done
}

_vary_packet_rate(){

    nodes=(100 200 300 400 500)

    for (( i = 0; i < 5; i++ ))
    do 
        
        first_data=1
        if [ $i -gt 0 ];
        then
            first_data=0
        fi
        echo packet_rate : ${nodes[i]} : $first_data
        # echo "$file_name --first_data=${first_data} --number_of_nodes=${nodes[i]}" 
        ./waf --run  "$file_name --changing_parameter=packet_rate --first_data=${first_data} --packet_rate=${nodes[i]}" 

    done
}   

_vary_nodes
_vary_flows
_vary_packet_rate

cd plotting

python3 task_plotter.py a2

cd -