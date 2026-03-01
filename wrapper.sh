#!/bin/bash
set -euo pipefail

# Determine rank id from common MPI/Slurm env vars
RANK_ID=${OMPI_COMM_WORLD_RANK:-${PMI_RANK:-${SLURM_PROCID:-0}}}

# Label (optional)
LABEL=${RUN_LABEL:-unknown}

OUTDIR="output/border-checkifarrived-inner/results_perf"
mkdir -p "${OUTDIR}"

OUTPUTFILE="${OUTDIR}/perf_stats_${LABEL}_rank_${RANK_ID}.txt"

# Run perf and write metrics to per-rank file; use commonly-available LLC events
exec perf stat -e LLC-loads,LLC-load-misses -o "${OUTPUTFILE}" "$@"