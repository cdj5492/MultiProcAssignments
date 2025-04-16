#!/bin/bash
#

# This is an example bash script that is used to submit a job
# to the cluster.
#
# Typcially, the # represents a comment. However, #SBATCH is
# interpreted by SLURM to give an option from above. As you 
# will see in the following lines, it is very useful to provide
# that information here, rather than the command line.

# Name of the job - You MUST use a unique name for the job
#SBATCH -J rt_mpi

# Standard out and Standard Error output files
#SBATCH -o std/rt_mpi_%j.out
#SBATCH -e std/rt_mpi_%j.err

# In order for this to send emails, you will need to remove the
# space between # and SBATCH for the following 2 commands.
# Specify the recipient of the email
# SBATCH --mail-user=abc1234@rit.edu

# Notify on state change: BEGIN, END, FAIL or ALL
# SBATCH --mail-type=ALL

# spack configuration help
#SBATCH --partition=kgcoe-mps
#SBATCH --account=kgcoe-mps
#SBATCH --get-user-env
# number of tasks 
#SBATCH --ntasks=64
# maximum run duration (your peers will thank you)
#SBATCH --time=0-1:0:0
# maximum memory per CPU
#SBATCH --mem-per-cpu=4000M

#
# Your job script goes below this line.
#

# This preps the system with all the needed libraries and resources
spack env activate cmpe-655

# Place your srun command here
# Notice that you have to provide the number of processes that
# are needed. This number needs to match the number of cores
# indicated by the -n option. If these do not, your results will
# not be valid or you may have wasted resources that others could
# have used.

# The following commands can be used as a starting point for 
# timing. Each example below needs to be modified to fit your parameters
# accordingly. You will also need to modify the config to point to
# box.xml to render the complex scene.
# **********************************************************************
# MAKE SURE THAT YOU ONLY HAVE ONE OF THESE UNCOMMENTED AT A TIME!
# **********************************************************************
# Sequential
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p none 

# Static Strips
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_strips_vertical

# Static Cycles
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 1
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 5
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 10 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 20
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 27
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 40
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 50
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 100
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 400
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 80
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 320
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 640
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 1280
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 160
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 350
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_cycles_horizontal -cs 650

# Static Blocks
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p static_blocks 

# Dynamic
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 1 -bw 1 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 15 -bw 15 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 25 -bw 25 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 50 -bw 50 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 75 -bw 75 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 100 -bw 100 

# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 11 -bw 1 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 1 -bw 10 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 7 -bw 13 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 5 -bw 29 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 9 -bw 110 
# srun -n $SLURM_NPROCS raytrace_mpi -h 5000 -w 5000 -c configs/twhitted.xml -p dynamic -bh 70 -bw 1 

# twhitted is simple scene, box is complex scene



