#!/bin/bash
#SBATCH --mem=0
#SBATCH --partition dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH -t 00:10:00
#SBATCH --exclusive

EXEC=./bin/parallel
# Wrapper used to run perf per-rank. Can be overridden in environment.
WRAPPER=${WRAPPER:-./wrapper.sh}

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
    # Run under wrapper so perf can collect single-rank stats as well
    ${WRAPPER} ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0
else
    # Launch MPI ranks under wrapper so perf runs on each rank
    mpirun ${MPI_ARGS} -np ${TOTAL_TASKS} ${WRAPPER} ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0 -e 10 -E 2
fi
