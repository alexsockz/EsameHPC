#!/bin/bash
set -euo pipefail

# Executable and wrapper (no spaces around =)
EXEC="./bin/parallel"
WRAPPER="./wrapper.sh"

# Defaults if not provided by environment
: ${TOTAL_RANKS:=1}
: ${SLURM_NTASKS_PER_NODE:=1}
: ${SLURM_CPUS_PER_TASK:=1}

# Run under mpirun, mapping by ppr per node and reserving PEs for threads
mpirun -np "${TOTAL_RANKS}" --map-by ppr:${SLURM_NTASKS_PER_NODE}:node:PE=${SLURM_CPUS_PER_TASK} \
	"${WRAPPER}" "${EXEC}" -x "${GRID_SIZE_X}" -y "${GRID_SIZE_Y}" -n 500 -p 0 -e 500 -E 25 -f 0.05 -m 1 -v 0 -o 0 >> run.out 2>&1