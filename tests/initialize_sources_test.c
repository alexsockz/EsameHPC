// File: /home/alessiovalle/GitRepos/esame-hpc/tests/initialize_sources_test.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>
#include "stencil_template_parallel.h"

// Helper to check if a source is in the buffer region
int is_in_buffer(const vec2_t src, const vec2_t mysize)
{
    // In memory_allocate, the plane is allocated as [mysize[_x_]] x [mysize[_y_] + 2]
    // The buffer is at y=0 and y=mysize[_y_]+1 (ghost layers)
    // Valid sources must have y in [1, mysize[_y_]]
    return (src[_y_] == 0 || src[_y_] == mysize[_y_] + 1);
    //it will be perview of the energy injection to check if it falls in the buffers in the end
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    MPI_Comm Comm = MPI_COMM_WORLD;
    int Me, Ntasks;
    MPI_Comm_rank(Comm, &Me);
    MPI_Comm_size(Comm, &Ntasks);

    printf("Test: Distribute 10 sources over 4x4 local plane\n");
    {
        vec2_t mysize = {4, 4};
        int Nsources = 10;
        int Nsources_local = 0;
        vec2_t *Sources_local = NULL;

        int ret = initialize_sources(Me, Ntasks, &Comm, mysize, Nsources, &Nsources_local, &Sources_local);
        assert(ret == 0);

        // Each local source must be within the valid region (not in halo/buffer rows)
        for (int i = 0; i < Nsources_local; ++i)
        {
            // x in [0, mysize[_x_]-1], y in [1, mysize[_y_]]
            assert(Sources_local[i][_x_] >= 0 && Sources_local[i][_x_] < mysize[_x_]);
            assert(Sources_local[i][_y_] >= 1 && Sources_local[i][_y_] <= mysize[_y_]);
            // Should not be in buffer/halo rows
            assert(!is_in_buffer(Sources_local[i], mysize));
        }

        free(Sources_local);
    }

    printf("Test: No sources\n");
    {
        vec2_t mysize = {8, 8};
        int Nsources = 0;
        int Nsources_local = -1;
        vec2_t *Sources_local = (vec2_t *)0xdeadbeef;

        int ret = initialize_sources(Me, Ntasks, &Comm, mysize, Nsources, &Nsources_local, &Sources_local);
        assert(ret == 0);
        assert(Nsources_local == 0);
        // Should not allocate any memory
        assert(Sources_local == (vec2_t *)0xdeadbeef || Sources_local == NULL);
    }

    // Test: Large domain, many sources
    // {
    //     vec2_t mysize = {100, 100};
    //     int Nsources = 1000;
    //     int Nsources_local = 0;
    //     vec2_t *Sources_local = NULL;

    //     int ret = initialize_sources(Me, Ntasks, &Comm, mysize, Nsources, &Nsources_local, &Sources_local);
    //     assert(ret == 0);

    //     for (int i = 0; i < Nsources_local; ++i) {
    //         assert(Sources_local[i][_x_] >= 0 && Sources_local[i][_x_] < mysize[_x_]);
    //         assert(Sources_local[i][_y_] >= 1 && Sources_local[i][_y_] <= mysize[_y_]);
    //         assert(!is_in_buffer(Sources_local[i], mysize));
    //     }

    //     free(Sources_local);
    // }

    printf("Test: All sources to one task (simulate by setting Ntasks=1)\n");
    if (Ntasks == 1)
    {
        vec2_t mysize = {5, 5};
        int Nsources = 7;
        int Nsources_local = 0;
        vec2_t *Sources_local = NULL;

        int ret = initialize_sources(Me, 1, &Comm, mysize, Nsources, &Nsources_local, &Sources_local);
        assert(ret == 0);
        assert(Nsources_local == Nsources);

        for (int i = 0; i < Nsources_local; ++i)
        {
            assert(Sources_local[i][_x_] >= 0 && Sources_local[i][_x_] < mysize[_x_]);
            assert(Sources_local[i][_y_] >= 1 && Sources_local[i][_y_] <= mysize[_y_]);
            assert(!is_in_buffer(Sources_local[i], mysize));
        }

        free(Sources_local);
    }

    printf("Test: As many sources as plane size, check for duplicates and buffer placement\n");
    {
        vec2_t mysize = {6, 6};
        int Nsources = mysize[_x_] * mysize[_y_];
        int Nsources_local = 0;
        vec2_t *Sources_local = NULL;

        int ret = initialize_sources(Me, Ntasks, &Comm, mysize, Nsources, &Nsources_local, &Sources_local);
        assert(ret == 0);

        // Gather all sources from all tasks to rank 0
        int *all_counts = NULL;
        int *displs = NULL;
        vec2_t *all_sources = NULL;
        if (Me == 0)
        {
            all_counts = malloc(Ntasks * sizeof(int));
            displs = malloc(Ntasks * sizeof(int));
        }
        MPI_Gather(&Nsources_local, 1, MPI_INT, all_counts, 1, MPI_INT, 0, Comm);

        int total_sources = 0;
        if (Me == 0)
        {
            displs[0] = 0;
            for (int i = 0; i < Ntasks; ++i)
            {
                if (i > 0)
                    displs[i] = displs[i - 1] + all_counts[i - 1];
                total_sources += all_counts[i];
            }
            all_sources = malloc(total_sources * sizeof(vec2_t));
            // Multiply counts and displs by sizeof(vec2_t) for MPI_BYTE
            for (int i = 0; i < Ntasks; ++i)
            {
                all_counts[i] *= sizeof(vec2_t);
                displs[i] *= sizeof(vec2_t);
            }
        }

        MPI_Gatherv(Sources_local, Nsources_local * sizeof(vec2_t), MPI_BYTE,
                    all_sources, all_counts, displs, MPI_BYTE, 0, Comm);

        if (Me == 0)
        {
            // Check all sources are within valid region and not in buffer/halo rows
            for (int i = 0; i < total_sources; ++i)
            {
                assert(all_sources[i][_x_] >= 0 && all_sources[i][_x_] < mysize[_x_]);
                assert(all_sources[i][_y_] >= 1 && all_sources[i][_y_] <= mysize[_y_]);
                assert(!is_in_buffer(all_sources[i], mysize));
            }
            // Check for duplicates, should there be no duplicates?
            // for (int i = 0; i < total_sources; ++i)
            // {
            //     for (int j = i + 1; j < total_sources; ++j)
            //     {
            //         assert(!(all_sources[i][_x_] == all_sources[j][_x_] &&
            //                  all_sources[i][_y_] == all_sources[j][_y_]));
            //     }
            // }
            free(all_counts);
            free(displs);
            free(all_sources);
        }

        free(Sources_local);
    }

    MPI_Finalize();
    if (Me == 0)
        printf("All initialize_sources tests passed.\n");
    return 0;
}