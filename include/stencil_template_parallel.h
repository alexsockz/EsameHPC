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

int get_total_energy(plane_t*,
                            double*);

int output_energy_stat(int,
                       plane_t *,
                       double,
                       int,
                       MPI_Comm *);


/* ==========================================================================
   =                                                                        =
   =   Initialization                                                       =
   ========================================================================== */

int initialize_sources(int,
                       int,
                       MPI_Comm *,
                       const uint[2],
                       int,
                       int *,
                       vec2_t **);

int initialize(MPI_Comm *,
               int,
               int,
               int,
               char **,
               vec2_t *,
               vec2_t *,
               int *,
               int *,
               int *,
               int *,
               int *,
               int *,
               int *,
               vec2_t **,
               double *,
               plane_t *,
               buffers_t *,
               buffers_t *);

/* ==========================================================================
   =                                                                        =
   =   Update                                                               =
   ========================================================================== */

extern int inject_energy(const int,
                         const int,
                         const vec2_t *,
                         const double,
                         plane_t *,
                         const vec2_t);

extern int update_plane(const int,
                        const vec2_t,
                        const plane_t *,
                        plane_t *);

/* ==========================================================================
   =                                                                        =
   =   Memory managment                                                     =
   ========================================================================== */

int memory_allocate(const int *,
                    buffers_t *,
                    buffers_t*,
                    plane_t *);

int memory_release(plane_t *, buffers_t *, buffers_t *);

/* ==========================================================================
   =                                                                        =
   =   Utils                                                                =
   ========================================================================== */

uint simple_factorization(uint, int *, uint **);

