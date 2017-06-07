#!/bin/bash
#script che elimina tutta la memoria, sem, code con permessi 666

for ipc in $(echo $(ipcs -m | grep 666 | cut -d ' ' -f2) )
do
	ipcrm -m $ipc
done

for ipc in $(echo $(ipcs -q | grep 666 | cut -d ' ' -f2) )
do
	ipcrm -q $ipc
done

for ipc in $(echo $(ipcs -s | grep 666 | cut -d ' ' -f2) )
do
	ipcrm -s $ipc
done

