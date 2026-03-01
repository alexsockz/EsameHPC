#!/bin/bash
#SBATCH --mem=0
#SBATCH --job-name=perf_scaling
#SBATCH --partition=dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH -t 00:30:00
#SBATCH --exclusive
#SBATCH --job-name=perf_runner
#SBATCH --nodes=16
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=14

set -euo pipefail

# Executable and wrapper (no spaces around =)

# If running under Slurm these are usually set; fall back to sensible defaults.
# Try SLURM_NNODES, otherwise use SLURM_JOB_NUM_NODES, otherwise 1.
SLURM_NNODES=${SLURM_NNODES:-${SLURM_JOB_NUM_NODES:-1}}

SLURM_NTASKS_PER_NODE=${SLURM_NTASKS_PER_NODE:-8}
SLURM_CPUS_PER_TASK=${SLURM_CPUS_PER_TASK:-14}

# Total MPI ranks
TOTAL_RANKS=$(( SLURM_NTASKS_PER_NODE * SLURM_NNODES ))
GRID_SIZE_X=${GRID_SIZE_X:-15000}
GRID_SIZE_Y=${GRID_SIZE_Y:-15000}
EXEC="./bin/parallel"
WRAPPER="./wrapper.sh"

## Defaults handled above; removed redundant fallbacks

# Load modules to match runtime environment
module purge
module load openmpi/4.1.6--gcc--12.2.0

# Run under mpirun, mapping by ppr per node and reserving PEs for threads
mpirun -np "${TOTAL_RANKS}" --map-by ppr:${SLURM_NTASKS_PER_NODE}:node:PE=${SLURM_CPUS_PER_TASK} \
	"${WRAPPER}" "${EXEC}" -x "${GRID_SIZE_X}" -y "${GRID_SIZE_Y}" -n 500 -p 0 -e 500 -E 25 -f 0.05 -m 1 -v 0 -o 0 >> run.out 2>&1