#!/bin/bash
#SBATCH --job-name=strong_scaling
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=UTS25_Tornator_0
#SBATCH --time=00:30:00
#SBATCH --nodes=16
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=14
#SBATCH --exclusive

# Paths
OMPI_DIR=$HOME/openmpi-5
EXEC=./bin/stencil_parallel
WRAPPER=./wrapper.sh

# Fallbacks when running the script directly (not via sbatch)
# Use SBATCH values when available, otherwise fall back to the SBATCH header defaults
NTASKS_PER_NODE=${SLURM_NTASKS_PER_NODE:-8}
CPUS_PER_TASK=${SLURM_CPUS_PER_TASK:-14}
GRID=${GRID:-1000}

# Environment
module purge
module load gcc/12.2.0
export OMPI_DIR=$HOME/openmpi-5
export PATH=$OMPI_DIR/bin:$PATH
export LD_LIBRARY_PATH=$OMPI_DIR/lib:$HOME/lib:$LD_LIBRARY_PATH

# Performance Settings
export OMP_NUM_THREADS=${CPUS_PER_TASK}
export OMP_PLACES=cores
export OMP_PROC_BIND=close

echo "Starting MPI Strong Scaling..."

for NODES in 1 2 4 8 16; do
    TOTAL_RANKS=$(( NODES * NTASKS_PER_NODE ))
    echo "Running on $NODES nodes ($TOTAL_RANKS total ranks)..."

    mpirun -np $TOTAL_RANKS \
    --map-by ppr:${NTASKS_PER_NODE}:node:PE=${CPUS_PER_TASK} \
    $WRAPPER ${EXEC} -x $GRID -y $GRID -n 500 -p 0 -e 500 -E 25 -f 0.05 -m 1 -v 0 -o 0 >> run.out
done