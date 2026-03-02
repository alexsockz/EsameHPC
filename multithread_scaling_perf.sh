#!/bin/bash

echo "Threads scaling: single node with multiple threads"

NODES=1
N_TASKS_PER_NODE=1
TOTAL_TASKS=1
N_STEPS=500
GRID_SIZE_X=32768
GRID_SIZE_Y=32768


for OMP_THREADS in 1 2 4 8 16 32 56 84 112; do
    JOB_NAME="thread_scaling_${OMP_THREADS}_threads"
    PERF_OUTPUT="output/border-checkifarrived-inner-32k/perf/${JOB_NAME}.perf"

    sbatch --export=ALL,GRID_SIZE_X=${GRID_SIZE_X},GRID_SIZE_Y=${GRID_SIZE_Y},N_STEPS=${N_STEPS},OMP_THREADS=${OMP_THREADS},JOB_NAME=${JOB_NAME},TOTAL_TASKS=${TOTAL_TASKS},PERF=1,PERF_OUTPUT=${PERF_OUTPUT} \
    --nodes=${NODES} \
    --ntasks-per-node=${N_TASKS_PER_NODE} \
    --cpus-per-task=${OMP_THREADS} \
    --job-name=${JOB_NAME} \
    go_dcgp.sh
done

echo "All threads scaling jobs submitted"
