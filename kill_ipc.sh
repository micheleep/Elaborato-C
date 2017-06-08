#!/bin/bash

ME=`whoami`

# vado a prendermi le ipcs che ho creato io
IPCS_S=`ipcs -s | egrep "0x[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`
IPCS_M=`ipcs -m | egrep "0x[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`
IPCS_Q=`ipcs -q | egrep "0x[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`

# rimuovo tutte le zone di memoria condivise che ho creato
for id in $IPCS_M; do
  ipcrm -m $id;
done

# rimuovo tutti i semafori che ho creato
for id in $IPCS_S; do
  ipcrm -s $id;
done

# rimuovo tutte le code dei messaggi che ho creato
for id in $IPCS_Q; do
  ipcrm -q $id;
done
