#!/bin/bash
#SBATCH --mem=0
#SBATCH --partition=dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH -t 00:30:00
#SBATCH --exclusive
#SBATCH --job-name=perf_runner
#SBATCH --nodes=16
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=14


# Export runtime vars used by perf_runner.sh (adjust as needed)
export SLURM_NTASKS_PER_NODE=${SLURM_NTASKS_PER_NODE:-8}
export SLURM_CPUS_PER_TASK=${SLURM_CPUS_PER_TASK:-14}
export TOTAL_RANKS=$(( SLURM_NTASKS_PER_NODE * SLURM_NNODES ))
export GRID_SIZE_X=${GRID_SIZE_X:-15000}
export GRID_SIZE_Y=${GRID_SIZE_Y:-15000}

# Load modules to match runtime environment
module purge
module load openmpi/4.1.6--gcc--12.2.0

# Run the launcher (it uses mpirun internally). Using srun to allocate resources.
srun --mpi=pmix_v3 ./perf_runner.sh
