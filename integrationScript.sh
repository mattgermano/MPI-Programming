#!/bin/bash

#PBS -S /bin/bash
#PBS -o pbs_integration_out.dat
#PBS -j oe
#PBS -l nodes=1:ppn=40

cd $PBS_O_WORKDIR

mpirun -np 300 -machinefile $PBS_NODEFILE ./integration 0 500 50000000
