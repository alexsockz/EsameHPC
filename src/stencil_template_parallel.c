
/*
 *
 *  mysizex   :   local x-extendion of your patch
 *  mysizey   :   local y-extension of your patch
 *
 */

#include "stencil_template_parallel.h"

// ------------------------------------------------------------------
// ------------------------------------------------------------------

int main(int argc, char **argv)
{
  MPI_Comm myCOMM_WORLD;
  int Rank;
  int Ntasks;
  int neighbours[4];

  int Niterations;
  int periodic;
  int verbose;
  vec2_t S, N;

  int Nsources;
  int Nsources_local;
  vec2_t *Sources_local;
  double energy_per_source;

  plane_t planes[2];
  buffers_t buffers[2]; // old new, each has 4
  buffers_t border_ptr[2];
  int output_energy_stat_perstep;
  register double alpha = 0.6;
  register double alpha_inverse = 1 / 4.0 * (1 - alpha);
  /* initialize MPI envrionment */
  {
    int level_obtained;

    // NOTE: change MPI_FUNNELED if appropriate
    //
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &level_obtained);
    if (level_obtained < MPI_THREAD_MULTIPLE)
    {
      printf("MPI_thread level obtained is %d instead of %d\n",
             level_obtained, MPI_THREAD_MULTIPLE);
      MPI_Finalize();
      exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
    MPI_Comm_size(MPI_COMM_WORLD, &Ntasks);
    MPI_Comm_dup(MPI_COMM_WORLD, &myCOMM_WORLD);
  }

  /* argument checking and setting */
  int ret = initialize(&myCOMM_WORLD, Rank, Ntasks, argc, argv, &S, &N, &periodic, &output_energy_stat_perstep, &verbose,
                       neighbours, &Niterations,
                       &Nsources, &Nsources_local, &Sources_local, &energy_per_source,
                       &planes[0], &buffers[0], border_ptr);
  printf("%d\n", Niterations);
  if (ret)
  {
    printf("task %d is opting out with termination code %d\n",
           Rank, ret);
    fflush(stdout);

    MPI_Finalize();
    return 0;
  }

  int current = OLD;
  double t1 = MPI_Wtime(); /* take wall-clock time */

  // somehow fit this in a cycle
  //  important for the future
  //  int i, j, k;
  //  #pragma omp parallel private(i,k) means that i and k will be unique for each thread and not shared

  for (int iter = 0; iter < Niterations; ++iter)
  {
    bool injected = false;
    if (verbose)
    {
      printf("TASK%d: beforeOMP\n", Rank);
      fflush(stdout);
    }
    MPI_Request reqs[8];

    /* new energy from sources */

    /* -------------------------------------- */

    // [A] fill the buffers, and/or make the buffers' pointers pointing to the correct position

    // [B] perfoem the halo communications
    //     (1) use Send / Recv
    //     (2) use Isend / Irecv
    //         --> can you overlap communication and compution in this way?

    // [C] copy the haloes data

    /* --------------------------------------  */
    /* update grid points */

#pragma omp parallel num_threads(9)
    {
      int myid = omp_get_thread_num();

      if (myid < 4)
      {

        // #define _x_ 0
        // #define _y_ 1

        // #define NORTH 0  _x_
        // #define SOUTH 1  _x_
        // #define EAST 2   _y_
        // #define WEST 3   _y_
        // incoming from, so if the buffer sent is south it needs to be put in in north
        MPI_Status status;
        int x_or_y = myid >> 1;                     // 0 if 0 or 1 and 1 if 2 or 3
        int source = (myid & 2) | (1 - (myid & 1)); // inverts 0 to 1 or 2 to 3 and vice versa
        // check the various pointers of this line
        // current or not current? not sure, i think not current aka next
        //  maybe change tag to current iteration number
        if (verbose)
        {
          printf("TASK%d: thread %d: waiting for Mpi_recv from %d, source %d \n", Rank, myid, neighbours[source], source);
          fflush(stdout);
        }

        MPI_Recv(buffers[!current][myid], N[x_or_y], MPI_DOUBLE, neighbours[source], BORDER_MESSAGE_TAG, myCOMM_WORLD, &status);
        if (verbose)
        {
          printf("TASK%d: thread %d: recieved from %d\n", Rank, myid, neighbours[source]);
          fflush(stdout);
        }
      }
      // TODO parallelize injection, but maybe not so worth it
      // do it only IF Nsources >>> nthread

#pragma omp masked filter(8)
      {
        if (verbose)
        {
          printf("TASK%d: thread %d: injecting\n", Rank, myid);
          fflush(stdout);
        }

        ret = inject_energy(periodic, Nsources_local, Sources_local, energy_per_source, &planes[current], N);
        if (ret == 0)
        {
#pragma omp atomic write
          injected = true;
#pragma omp flush(injected)
        }
        if (verbose)
        {
          printf("TASK%d: thread %d: injected\n", Rank, myid);
          fflush(stdout);
        }

        if (verbose)
        {
          printf("TASK%d: thread %d: updating plane\n", Rank, myid);
          fflush(stdout);
        }
        update_plane(periodic, N, &planes[OLD], &planes[NEW]);
        if (verbose)
        {
          printf("TASK%d: thread %d: updated plane\n", Rank, myid);
          fflush(stdout);
        }
      }

      if (myid > 3 && myid < 8)
      {
        /* busy-wait */
        int val = 0;
        while (1)
        {
#pragma omp atomic read
          val = injected;
#pragma omp flush(injected)
          if (val)
            break;
        }
        int work_direction = myid - 4;
        if (verbose)
        {
          printf("TASK%d: thread %d: calculating border %d\n", Rank, myid, work_direction);
          fflush(stdout);
        }
        int x_or_y = work_direction >> 1; // 0 if 0 or 1 and 1 if 2 or 3
        double const *old_border = border_ptr[current][work_direction];
        double const *old_buffer = buffers[current][work_direction];
        double *new_border = border_ptr[!current][work_direction];
        int next_row = N[_x_];
        if (x_or_y)
        {
          int skips = 0;
          int plus_or_minus_one = -1;
          // TODO either do this or switch to a personalized MPI_TYPE
          double *momentary_buffer = (double *)malloc(N[_y_] * sizeof(double));

          if (work_direction == WEST)
            plus_or_minus_one = 1;
          for (uint i = 0; i < N[x_or_y]; i++)
          {
            double result = old_border[skips] * alpha;
            // perpendicular to direction
            double sum_i = (old_buffer[i] + old_border[skips + plus_or_minus_one]) * alpha_inverse;
            // parallel
            // might be illegal
            double sum_j = (old_border[skips - next_row] + old_border[skips + next_row]) * alpha_inverse;
            result += (sum_i + sum_j);
            new_border[skips] = result;
            momentary_buffer[i] = result;

            skips += next_row;
          }
          if (verbose)
          {
            printf("TASK%d: thread %d: calculated border %d\n", Rank, myid, work_direction);
            fflush(stdout);
          }
          MPI_Isend(momentary_buffer, N[x_or_y], MPI_DOUBLE, neighbours[work_direction], BORDER_MESSAGE_TAG, myCOMM_WORLD, &reqs[work_direction]);
          if (verbose)
          {
            printf("TASK%d: thread %d: sent border %d\n", Rank, myid, work_direction);
            fflush(stdout);
          }
        }
        else
        {
          for (uint i = 1; i < N[x_or_y] - 1; i++)
          {
            double result = old_border[i] * alpha;
            // parallel to dircetion
            double sum_i = (old_border[i - 1] + old_border[i + 1]) * alpha_inverse;
            // perpendicular
            double sum_j = (old_buffer[i] + old_border[i + next_row]) * alpha_inverse;
            result += (sum_i + sum_j);
            new_border[i] = result;
          }
          if (verbose)
          {
            printf("TASK%d: thread %d: calculated border\n", Rank, myid);
            fflush(stdout);
          }

          MPI_Isend(new_border, N[x_or_y], MPI_DOUBLE, neighbours[work_direction], BORDER_MESSAGE_TAG, myCOMM_WORLD, &reqs[work_direction]);

          if (verbose)
          {
            printf("TASK%d: thread %d: sent border\n", Rank, myid);
            fflush(stdout);
          }
        }
      }
    }
    /* output if needed */
    if (output_energy_stat_perstep)
      output_energy_stat(iter, &planes[!current], (iter + 1) * Nsources * energy_per_source, Rank, &myCOMM_WORLD);

    /* swap plane indexes for the new iteration */
    current = !current;
  }

  t1 = MPI_Wtime() - t1;
  output_energy_stat(-1, &planes[!current], Niterations * Nsources * energy_per_source, Rank, &myCOMM_WORLD);

  memory_release(planes, buffers, border_ptr);

  MPI_Finalize();
  return 0;
}