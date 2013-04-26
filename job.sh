#!/bin/bash
#PBS -q batch
#PBS -N ./bin/nbody
#PBS -r n
#PBS -k oe
#PBS -l nodes=1:ppn=4
#PBS -l walltime=999:00:00
echo Working directory is $PBS_O_WORKDIR
echo Running on host `hostname`
echo Time is `date`
echo Directory is `pwd`
cd ~/assignment1/
echo Directory is `pwd`
mpiexec -hostfile hostfile -np 16 ./bin/nbody > nbody_output
