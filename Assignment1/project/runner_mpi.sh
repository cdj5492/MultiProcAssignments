#!/bin/bash
#

# This is an example bash script that is used to submit a job
# to the cluster.
#
# Typcially, the # represents a comment.However, #SBATCH is
# interpreted by SLURM to give an option from above. As you
# will see in the following lines, it is very useful to provide
# that information here, rather than the command line.

# Name of the job - You MUST use a unique name for the job
#SBATCH -J x6d15c10

# Standard out and Standard Error output files
# Variable	Example				Description
# %x		#SBATCH --output=%x_%j.out	Fill in job name (set by --job-name)
# %j		#SBATCH --error=%x_%j.err	Fill in job ID (set by Slurm)
# %%		#SBATCH --output=%x_20%%.out	Escape percent sign; creates <job_name>_20%.out
#SBATCH -o std/%x-%j.out
#SBATCH -e std/%x-%j.err

# In order for this to send emails, you will need to remove the
# space between # and SBATCH for the following 2 commands.
# Specify the recipient of the email
# SBATCH --mail-user=abc1234@rit.edu

# Notify on state change: BEGIN, END, FAIL or ALL
# SBATCH --mail-type=ALL

# Here, we specify the partition, the number of cores to use,
# the amount of memory we would like per core, and set a
# maximum runtime duration to avoid any hanging runs.
#SBATCH --partition=kgcoe-mps
#SBATCH --account=kgcoe-mps
#SBATCH --get-user-env
#SBATCH --mem=1G
#SBATCH --time=0-1:0:0
#SBATCH --ntasks=13

#
# Your job script goes below this line.
#

# If the job that you are submitting is not sequential,
# then you MUST provide this line...it tells the node(s)
# that you want to use this implementation of MPI. If you
# omit this line, your results will indicate failure.
spack env activate cmpe-655

# Place your srun command here
# Notice that you have to provide the number of processes that
# are needed. This number needs to match the number of cores
# indicated by the -n option. If these do not, your results will
# not be valid or you may have wasted resources that others could
# have used. Using $SLURM_NPROCS guarantees a match.
srun -n $SLURM_NPROCS mpi_hh -d 15 -c 10
# srun -n 11 mpi_hh -d 30 -c 100
