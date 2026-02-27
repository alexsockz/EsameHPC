#include "stencil_template_parallel.h"

int initialize_sources(int Me,
                       int Ntasks,
                       MPI_Comm *Comm,
                       vec2_t const mysize,
                       int Nsources,
                       int *Nsources_local,
                       vec2_t **Sources)
{
  //the first task decides each task i how many sources does it have
  //srand48(time(NULL) ^ Me);
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
  // should the sources be at maximum 1 per point?
  if (nlocal > 0)
  {
    vec2_t *restrict helper = (vec2_t *)malloc(nlocal * sizeof(vec2_t));
    for (int s = 0; s < nlocal; s++)
    {
      // use 1-based interior coordinates for both x and y
      helper[s][_x_] = 1 + (uint)lrand48() % mysize[_x_];
      helper[s][_y_] = 1 + (uint)lrand48() % mysize[_y_];
    }

    *Sources = helper;
  }

  free(tasks_with_sources);

  return 0;
}
