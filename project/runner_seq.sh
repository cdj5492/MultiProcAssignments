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
#SBATCH -J p1d15c10

# Standard out and Standard Error output files
# Variable	Example				Description
# %x		#SBATCH --output=%x_%j.out	Fill in job name (set by --job-name)
# %j		#SBATCH --error=%x_%j.err	Fill in job ID (set by Slurm)
# %%		#SBATCH --output=%x_20%%.out	Escape percent sign; creates <job_name>_20%.out
#SBATCH --output=std/%x-%j.out
#SBATCH --error=std/%x-%j.err

# In order for this to send emails, you will need to remove the
# space between # and SBATCH for the following 2 commands.
# Specify the recipient of the email
# SBATCH --mail-user=abc1234@rit.edu

# Notify on state change: BEGIN, END, FAIL or ALL
# SBATCH --mail-type=ALL

# Multiple options can be used on the same line as shown below.
# Here, we set the partition, number of cores to use, and the
# number of nodes to spread the jobs over.
# Since this is a sequential(one process) job, we only need one
# core and one node.
#SBATCH --partition=kgcoe-mps
#SBATCH --account=kgcoe-mps
#SBATCH --get-user-env
#SBATCH --mem=1G
#SBATCH --time=0-1:0:0
#SBATCH --ntasks=1

#
# Your job script goes below this line.
#

spack env activate cmpe-655

# Place your mpirun command here
srun -n $SLURM_NPROCS seq_hh -d 15 -c 10
