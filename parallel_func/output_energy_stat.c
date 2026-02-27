#include "stencil_template_parallel.h"

int output_energy_stat(int step, plane_t *plane, double budget, int Me, MPI_Comm *Comm)
{

  double system_energy = 0;
  double tot_system_energy = 0;
  get_total_energy(plane, &system_energy);

    MPI_Reduce(&system_energy, &tot_system_energy, 1, MPI_DOUBLE, MPI_SUM, 0, *Comm);

    /* compute total number of interior grid points across all ranks
      so the average-per-point is computed correctly on the root */
    long long local_points = (long long)plane->size[_x_] * (long long)plane->size[_y_];
    long long tot_points = 0;
    MPI_Reduce(&local_points, &tot_points, 1, MPI_LONG_LONG, MPI_SUM, 0, *Comm);

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
          tot_system_energy / (double)tot_points);
  }

  return 0;
}