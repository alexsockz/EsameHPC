#!/bin/bash -l
#SBATCH --job-name=openmp_test
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=UTS25_Tornator_0
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=112
#SBATCH --time=00:45:00
#SBATCH --exclusive
#SBATCH --mem=0

# Paths
OMPI_DIR=$HOME/openmpi-5
EXEC=./stencil_parallel

# Environment
module purge
module load gcc/12.2.0
export OMPI_DIR=$HOME/openmpi-5
export PATH=$OMPI_DIR/bin:$PATH
export LD_LIBRARY_PATH=$OMPI_DIR/lib:$HOME/lib:$LD_LIBRARY_PATH

# Performance Settings
export OMP_DISPLAY_AFFINITY=TRUE
export OMP_PLACES=cores
export OMP_PROC_BIND=close

echo "Starting OpenMP scaling test (1 MPI Rank)..."

# Scaling Loop (1 to 128 threads)
for THREADS in 1 2 4 8 16 32 56 84 112; do
    export OMP_NUM_THREADS=$THREADS

    echo "Running with $THREADS threads..."

    mpirun -np ${SLURM_NTASKS} \
        --report-bindings \
        --bind-to none \
        --tag-output \
        ${EXEC} -x 16384 -y 16384 -n 500 -p 0 -e 500 -E 25 -f 0.05 -m 1 >> run.out
done
