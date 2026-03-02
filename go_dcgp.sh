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
    # If PERF is set (non-empty and not 0) run the single-task executable under perf
    if [ -n "${PERF}" ] && [ "${PERF}" != "0" ]; then
        PERF_CMD=(perf stat -e cache-misses,cache-references)
        if [ -n "${PERF_OUTPUT}" ]; then
            PERF_CMD+=( -o "${PERF_OUTPUT}" )
        fi
        PERF_CMD+=( -- )
        "${PERF_CMD[@]}" ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0
    else
        ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0
    fi
else
    # For multi-rank runs, prefer perf in system-wide mode (-a) to capture distributed activity
    if [ -n "${PERF}" ] && [ "${PERF}" != "0" ]; then
        if [ -n "${PERF_OUTPUT}" ]; then
            perf stat -a -e cache-misses,cache-references -o "${PERF_OUTPUT}" -- \
                mpirun ${MPI_ARGS} -np ${TOTAL_TASKS} ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0 -e 10 -E 2
        else
            perf stat -a -e cache-misses,cache-references -- mpirun ${MPI_ARGS} -np ${TOTAL_TASKS} ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0 -e 10 -E 2
        fi
    else
        mpirun ${MPI_ARGS} -np ${TOTAL_TASKS} ${EXEC} -n ${N_STEPS} -x ${GRID_SIZE_X} -y ${GRID_SIZE_Y} -p 1 -o 0 -e 10 -E 2
    fi
fi
