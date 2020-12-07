#!/bin/bash

let "ripeti = 1"
while [ $ripeti -lt 3 ]
do
    clear
	ipcs
    sleep .3
done
echo "hurra!!"
