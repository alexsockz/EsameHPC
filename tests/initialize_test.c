#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>
#include "stencil_template_parallel.h"

// Helper to simulate argv
char **make_argv(const char *args[], int argc)
{
    char **argv = malloc(argc * sizeof(char *));
    for (int i = 0; i < argc; i++)
    {
        argv[i] = strdup(args[i]);
    }
    return argv;
}

void free_argv(char **argv, int argc)
{
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    MPI_Comm Comm = MPI_COMM_WORLD;
    int Me;
    int Ntasks;
    MPI_Comm_rank(Comm, &Me);
    MPI_Comm_size(Comm, &Ntasks);

    printf("Test: Default values, no arguments\n");
    {
        vec2_t S, N;
        int periodic, output_energy_stat, neighbours[4], Niterations, Nsources, Nsources_local;
        vec2_t *Sources_local;
        double energy_per_source;
        plane_t planes[2];
        buffers_t buffers[2];
        buffers_t border_ptr[2];

        char *args[] = {"prog"};
        char **argv_test = malloc(sizeof(char *) * 1);
        argv_test[0] = strdup(args[0]);

        int ret = initialize(&Comm, Me, Ntasks, 1, argv_test, &S, &N, &periodic, &output_energy_stat,
                             neighbours, &Niterations, &Nsources, &Nsources_local, &Sources_local,
                             &energy_per_source, planes, buffers, border_ptr);

        assert(ret == 0);
        assert(S[_x_] == 10000 && S[_y_] == 10000);
        assert(Nsources == 4);
        assert(Niterations == 1000);
        assert(energy_per_source == 1.0);
        assert(planes[OLD].size[_x_] > 0 && planes[OLD].size[_y_] > 0);
        assert(planes[OLD].data != NULL && planes[NEW].data != NULL);
        free(argv_test[0]);
        free(argv_test);
        free(planes[OLD].data);
        free(planes[NEW].data);
        free(buffers[OLD][EAST]);
        free(buffers[OLD][WEST]);
        free(buffers[NEW][EAST]);
        free(buffers[NEW][WEST]);
    }

    printf("Test: Minimum values\n");
    {
        vec2_t S, N;
        int periodic, output_energy_stat, neighbours[4], Niterations, Nsources, Nsources_local;
        vec2_t *Sources_local;
        double energy_per_source;
        plane_t planes[2];
        buffers_t buffers[2];
        buffers_t border_ptr[2];

        char *args[] = {"prog", "-x", "1", "-y", "1", "-e", "0", "-E", "0", "-n", "1", "-p 0", " "};
        char **argv_test = malloc(sizeof(char *) * 12);
        for (int i = 0; i < 12; i++)
            argv_test[i] = strdup(args[i]);

        int ret = initialize(&Comm, Me, Ntasks, 12, argv_test, &S, &N, &periodic, &output_energy_stat,
                             neighbours, &Niterations, &Nsources, &Nsources_local, &Sources_local,
                             &energy_per_source, planes, buffers, border_ptr);
        assert(ret == 0);
        assert(S[_x_] == 1 && S[_y_] == 1);
        assert(Nsources == 0);
        assert(Niterations == 1);
        assert(energy_per_source == 0.0);
        assert(planes[OLD].size[_x_] > 0 && planes[OLD].size[_y_] > 0);
        assert(planes[OLD].data != NULL && planes[NEW].data != NULL);

        for (int i = 0; i < 12; i++)
            free(argv_test[i]);
        free(argv_test);
        free(planes[OLD].data);
        free(planes[NEW].data);
        free(buffers[OLD][EAST]);
        free(buffers[OLD][WEST]);
        free(buffers[NEW][EAST]);
        free(buffers[NEW][WEST]);
    }

    printf("Test: Maximum values\n");
    {
        vec2_t S, N;
        int periodic, output_energy_stat, neighbours[4], Niterations, Nsources, Nsources_local;
        vec2_t *Sources_local;
        double energy_per_source;
        plane_t planes[2];
        buffers_t buffers[2];
        buffers_t border_ptr[2];
        char *args[] = {"prog", "-x", "10000", "-y", "10000", "-e", "100", "-E", "1000", "-n", "10000", "-p 1", " "};
        char **argv_test = malloc(sizeof(char *) * 12);
        for (int i = 0; i < 12; i++)
            argv_test[i] = strdup(args[i]);

        int ret = initialize(&Comm, Me, Ntasks, 12, argv_test, &S, &N, &periodic, &output_energy_stat,
                             neighbours, &Niterations, &Nsources, &Nsources_local, &Sources_local,
                             &energy_per_source, planes, buffers, border_ptr);

        assert(ret == 0);
        assert(S[_x_] == 10000 && S[_y_] == 10000);
        assert(Nsources == 100);
        assert(Niterations == 10000);
        assert(energy_per_source == 1000.0);
        assert(planes[OLD].size[_x_] > 0 && planes[OLD].size[_y_] > 0);
        assert(planes[OLD].data != NULL && planes[NEW].data != NULL);

        for (int i = 0; i < 12; i++)
            free(argv_test[i]);
        free(argv_test);
        free(planes[OLD].data);
        free(planes[NEW].data);
        free(buffers[OLD][EAST]);
        free(buffers[OLD][WEST]);
        free(buffers[NEW][EAST]);
        free(buffers[NEW][WEST]);
    }

    printf("Test: Periodic boundaries\n");
    {
        vec2_t S, N;
        int periodic, output_energy_stat, neighbours[4], Niterations, Nsources, Nsources_local;
        vec2_t *Sources_local;
        double energy_per_source;
        plane_t planes[2];
        buffers_t buffers[2];
        buffers_t border_ptr[2];
        char *args[] = {"prog", "-p", "1"};
        char **argv_test = malloc(sizeof(char *) * 3);
        for (int i = 0; i < 3; i++)
            argv_test[i] = strdup(args[i]);

        int ret = initialize(&Comm, Me, Ntasks, 3, argv_test, &S, &N, &periodic, &output_energy_stat,
                             neighbours, &Niterations, &Nsources, &Nsources_local, &Sources_local,
                             &energy_per_source, planes, buffers, border_ptr);

        assert(ret == 0);
        assert(periodic == 1);

        for (int i = 0; i < 3; i++)
            free(argv_test[i]);
        free(argv_test);
        free(planes[OLD].data);
        free(planes[NEW].data);
        free(buffers[OLD][EAST]);
        free(buffers[OLD][WEST]);
        free(buffers[NEW][EAST]);
        free(buffers[NEW][WEST]);
    }

    printf("Test: Output energy stat enabled\n");
    {
        vec2_t S, N;
        int periodic, output_energy_stat, neighbours[4], Niterations, Nsources, Nsources_local;
        vec2_t *Sources_local;
        double energy_per_source;
        plane_t planes[2];
        buffers_t buffers[2];
        buffers_t border_ptr[2];
        char *args[] = {"prog", "-o", "1"};
        char **argv_test = malloc(sizeof(char *) * 3);
        for (int i = 0; i < 3; i++)
            argv_test[i] = strdup(args[i]);

        int ret = initialize(&Comm, Me, Ntasks, 3, argv_test, &S, &N, &periodic, &output_energy_stat,
                             neighbours, &Niterations, &Nsources, &Nsources_local, &Sources_local,
                             &energy_per_source, planes, buffers, border_ptr);

        assert(ret == 0);
        assert(output_energy_stat == 1);

        for (int i = 0; i < 3; i++)
            free(argv_test[i]);
        free(argv_test);
        free(planes[OLD].data);
        free(planes[NEW].data);
        free(buffers[OLD][EAST]);
        free(buffers[OLD][WEST]);
        free(buffers[NEW][EAST]);
        free(buffers[NEW][WEST]);
    }

    MPI_Finalize();
    printf("All initialize tests passed.\n");
    return 0;
}