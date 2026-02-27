/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <omp.h>
#include <mpi.h>

#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3

#define SEND 0
#define RECV 1

#define OLD 0
#define NEW 1

#define _x_ 0
#define _y_ 1

#define GENERAL_USE_TAG 0
#define BORDER_MESSAGE_TAG 1

#define ALPHA 0.6

typedef unsigned int uint;

typedef uint vec2_t[2];
typedef double *restrict buffers_t[4]; //north south will point to matrix, east west will point to an array used as buffer

typedef struct
{
    double *restrict data;
    vec2_t size;
} plane_t;


/* ==========================================================================
   =                                                                        =
   =   Output                                                               =
   ========================================================================== */

inline int get_total_energy(const plane_t * plane,
                            double *energy);

int output_energy_stat(int step, plane_t const *plane, double budget, int Me, MPI_Comm *Comm);

inline void print_matrix(int rank, int Ntask, int x_size, int y_size, const double* matrix_ptr,
                                    const double**, MPI_Comm *Comm);

int dump(const double *data, const uint size[2], const char *filename, double *min, double *max);

/* ==========================================================================
   =                                                                        =
   =   Initialization                                                       =
   ========================================================================== */

int initialize_sources(int Me,
                       int Ntasks,
                       MPI_Comm *Comm,
                       vec2_t const mysize,
                       int Nsources,
                       int *Nsources_local,
                       vec2_t **Sources);

int initialize(MPI_Comm *Comm,
               int Me,        // the rank of the calling process
               int Ntasks,    // the total number of MPI ranks
               int argc,      // the argc from command line
               char **argv,   // the argv from command line
               vec2_t *S,     // the size of the plane
               vec2_t *N,     // two-uint array defining the MPI tasks' grid
               int *periodic, // periodic-boundary tag
               int *output_energy_stat,
               int *verbose,
               int *neighbours,  // four-int array that gives back the neighbours of the calling task
               int *Niterations, // how many iterations
               int *Nsources,    // how many heat sources
               int *Nsources_local,
               vec2_t **Sources_local,
               double *energy_per_source, // how much heat per source
               plane_t *planes,
               buffers_t *buffers,
               buffers_t *borders_ptr);

/* ==========================================================================
   =                                                                        =
   =   Update                                                               =
   ========================================================================== */

inline int inject_energy(const int periodic,
                         const int Nsources,
                         const vec2_t *Sources,
                         const double energy,
                         plane_t *plane,
                         const vec2_t N);

inline int update_plane(const int periodic,
                        const vec2_t N, // the grid of MPI tasks
                        const plane_t *oldplane,
                        plane_t *newplane);

int update_border(int myid, int iter, double const *old_border,double const *old_buffer, 
                    double *new_border, const vec2_t S, const int* neighbours,MPI_Comm *Comm, MPI_Request* reqs);

/* ==========================================================================
   =                                                                        =
   =   Memory managment                                                     =
   ========================================================================== */

int memory_allocate(buffers_t *buffers_ptr,
                    buffers_t *borders_ptr,
                    plane_t *planes_ptr);

int memory_release(plane_t *planes, buffers_t * buffers_ptr, buffers_t * border_ptr);

/* ==========================================================================
   =                                                                        =
   =   Utils                                                                =
   ========================================================================== */

uint simple_factorization(uint A, int *Nfactors, uint **factors);

