#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>
#include "stencil_template_parallel.h"

static void test_buffer_immutability()
{
    vec2_t mysize = {6, 6};
    plane_t planes[2];
    buffers_t buffers[2];
    buffers_t borders[2];
    int neighbours[4] = {-1, -1, -1, -1};

    planes[OLD].size[_x_] = mysize[_x_];
    planes[OLD].size[_y_] = mysize[_y_];
    planes[NEW].size[_x_] = mysize[_x_];
    planes[NEW].size[_y_] = mysize[_y_];

    int ret = memory_allocate(neighbours, buffers, borders, planes);
    assert(ret == 0);

    int x_size = mysize[_x_];
    int y_size = mysize[_y_];
    unsigned int frame_size = x_size * (y_size + 2);

    for (unsigned int k = 0; k < frame_size; ++k) {
        planes[OLD].data[k] = 1000.0 + k;
        planes[NEW].data[k] = 2000.0 + k;
    }
    for (int j = 0; j < y_size; ++j) {
        buffers[OLD][WEST][j] = -10.0 - j;
        buffers[OLD][EAST][j] = -20.0 - j;
        buffers[NEW][WEST][j] = -30.0 - j;
        buffers[NEW][EAST][j] = -40.0 - j;
    }

    double *snap_old_west = malloc(y_size * sizeof(double));
    double *snap_old_east = malloc(y_size * sizeof(double));
    double *snap_new_west = malloc(y_size * sizeof(double));
    double *snap_new_east = malloc(y_size * sizeof(double));
    double *snap_old_north = malloc(x_size * sizeof(double));
    double *snap_old_south = malloc(x_size * sizeof(double));

    for (int j = 0; j < y_size; ++j) {
        snap_old_west[j] = buffers[OLD][WEST][j];
        snap_old_east[j] = buffers[OLD][EAST][j];
        snap_new_west[j] = buffers[NEW][WEST][j];
        snap_new_east[j] = buffers[NEW][EAST][j];
    }
    double *north_ptr = buffers[OLD][NORTH];
    double *south_ptr = buffers[OLD][SOUTH];
    for (int i = 0; i < x_size; ++i) {
        snap_old_north[i] = north_ptr[i];
        snap_old_south[i] = south_ptr[i];
    }

    vec2_t N = {1, 1};
    ret = update_plane(0, N, &planes[OLD], &planes[NEW]);
    assert(ret == 0);

    for (int j = 0; j < y_size; ++j) {
        assert(buffers[OLD][WEST][j] == snap_old_west[j]);
        assert(buffers[OLD][EAST][j] == snap_old_east[j]);
        assert(buffers[NEW][WEST][j] == snap_new_west[j]);
        assert(buffers[NEW][EAST][j] == snap_new_east[j]);
    }
    for (int i = 0; i < x_size; ++i) {
        assert(north_ptr[i] == snap_old_north[i]);
        assert(south_ptr[i] == snap_old_south[i]);
    }

    free(snap_old_west);
    free(snap_old_east);
    free(snap_new_west);
    free(snap_new_east);
    free(snap_old_north);
    free(snap_old_south);

    free(planes[OLD].data);
    free(planes[NEW].data);
    free(buffers[OLD][EAST]);
    free(buffers[OLD][WEST]);
    free(buffers[NEW][EAST]);
    free(buffers[NEW][WEST]);
}

static void test_inject_energy_with_initialize_sources(int Me, int Ntasks, MPI_Comm *Comm)
{
    // This test only runs reliably in single-task mode; skip otherwise.
    if (Ntasks != 1) {
        if (Me == 0) printf("Skipping inject_energy test (requires single MPI task).\n");
        return;
    }

    vec2_t mysize = {8, 4}; // x,y interior sizes
    plane_t planes[1];
    buffers_t buffers[1];
    buffers_t borders[1];
    int neighbours[4] = {-1, -1, -1, -1};

    planes[0].size[_x_] = mysize[_x_];
    planes[0].size[_y_] = mysize[_y_];

    // reuse memory_allocate to get consistent layout
    int ret = memory_allocate(neighbours, buffers, borders, planes);
    assert(ret == 0);

    // zero the plane data
    uint fxsize = planes[0].size[_x_] + 2;
    uint fysize = planes[0].size[_y_] + 2;
    for (uint j = 0; j < fysize; ++j)
        for (uint i = 0; i < fxsize; ++i)
            planes[0].data[j * fxsize + i] = 0.0;

    // initialize sources: choose Nsources such that with Ntasks==1 they belong to this task
    int Nsources = 3;
    int Nsources_local = 0;
    vec2_t *Sources = NULL;
    ret = initialize_sources(Me, Ntasks, Comm, mysize, Nsources, &Nsources_local, &Sources);
    assert(ret == 0);
    assert(Nsources_local == Nsources);
    assert(Sources != NULL);

    double energy = 5.5;
    ret = inject_energy(0, Nsources_local, Sources, energy, &planes[0], (vec2_t){1,1});
    assert(ret == 0);

    // verify energy injected at the reported source positions
    uint fx = planes[0].size[_x_] + 2;
    for (int s = 0; s < Nsources_local; ++s) {
        int x = Sources[s][_x_];
        int y = Sources[s][_y_];
        double val = planes[0].data[y * fx + x];
        // initialize_sources produced y in [1..mysize_y], x in [0..mysize_x-1]
        assert(val == energy);
        // zero-check neighbors untouched (simple spot-check)
        if (x > 0) assert(planes[0].data[y * fx + (x-1)] == 0.0);
    }

    free(Sources);
    free(planes[0].data);
    free(buffers[0][EAST]);
    free(buffers[0][WEST]);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm Comm = MPI_COMM_WORLD;
    int Me, Ntasks;
    MPI_Comm_rank(Comm, &Me);
    MPI_Comm_size(Comm, &Ntasks);

    if (Me == 0) printf("Running additional tests...\n");

    test_buffer_immutability();
    test_inject_energy_with_initialize_sources(Me, Ntasks, &Comm);

    if (Me == 0) printf("Additional tests passed.\n");

    MPI_Finalize();
    return 0;
}