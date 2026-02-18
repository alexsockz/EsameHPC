#include "stencil_template_parallel.h"

int output_energy_stat(int step, plane_t *plane, double budget, int Me, MPI_Comm *Comm)
{

  double system_energy = 0;
  double tot_system_energy = 0;
  get_total_energy(plane, &system_energy);

  MPI_Reduce(&system_energy, &tot_system_energy, 1, MPI_DOUBLE, MPI_SUM, 0, *Comm);

  if (Me == 0)
  {
    if (step >= 0)
      printf(" [ step %4d ] ", step);
    fflush(stdout);

    printf("total injected energy is %g, "
           "system energy is %g "
           "( in avg %g per grid point)\n",
           budget,
           tot_system_energy,
           tot_system_energy / (plane->size[_x_] * plane->size[_y_]));
  }

  return 0;
}