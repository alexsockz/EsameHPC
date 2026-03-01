#!/bin/bash
#SBATCH --mem=0
#SBATCH --partition dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH -t 00:10:00
#SBATCH --exclusive

EXEC=./bin/parallel

# =======================================================
module purge
module load openmpi/4.1.6--gcc--12.2.0

export OMP_NUM_THREADS=$OMP_THREADS
export OMP_PLACES=cores
export OMP_PROC_BIND=close


# Build tuned MPI arguments for binding/mapping. Prefer SLURM-provided topology when available.
MPI_ARGS_DEFAULT="--bind-to core --report-bindings --rank-by core"

# If SLURM provides CPUs-per-task, map by socket with PE set to OMP threads (better hybrid mapping)
if [ -n "${SLURM_CPUS_PER_TASK}" ]; then
    MPI_ARGS_DEFAULT="${MPI_ARGS_DEFAULT} --map-by socket:PE=${SLURM_CPUS_PER_TASK}"
elif [ -n "${OMP_THREADS}" ]; then
    MPI_ARGS_DEFAULT="${MPI_ARGS_DEFAULT} --map-by socket:PE=${OMP_THREADS}"
else
    MPI_ARGS_DEFAULT="${MPI_ARGS_DEFAULT} --map-by socket"
fi

# Allow override from environment (MPI_ARGS), otherwise use MPI_ARGS_DEFAULT
: ${MPI_ARGS:=${MPI_ARGS_DEFAULT}}

if [[ ${TOTAL_TASKS} -eq 1 ]]; then
    ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0
else
    mpirun ${MPI_ARGS} -np ${TOTAL_TASKS} ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0 -e 10 -E 2
fi
