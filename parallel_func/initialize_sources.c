#include "stencil_template_parallel.h"

int initialize_sources(int Me,
                       int Ntasks,
                       MPI_Comm *Comm,
                       vec2_t mysize,
                       int Nsources,
                       int *Nsources_local,
                       vec2_t **Sources)
{

  srand48(time(NULL) ^ Me);
  int *tasks_with_sources = (int *)malloc(Nsources * sizeof(int));

  if (Me == 0)
  {
    for (int i = 0; i < Nsources; i++)
      tasks_with_sources[i] = (int)lrand48() % Ntasks;
  }

  MPI_Bcast(tasks_with_sources, Nsources, MPI_INT, 0, *Comm);

  int nlocal = 0;
  for (int i = 0; i < Nsources; i++)
    nlocal += (tasks_with_sources[i] == Me);
  *Nsources_local = nlocal;

  if (nlocal > 0)
  {
    vec2_t *restrict helper = (vec2_t *)malloc(nlocal * sizeof(vec2_t));
    for (int s = 0; s < nlocal; s++)
    {
      helper[s][_x_] = 1 + lrand48() % mysize[_x_];
      helper[s][_y_] = 1 + lrand48() % mysize[_y_];
    }

    *Sources = helper;
  }

  free(tasks_with_sources);

  return 0;
}
