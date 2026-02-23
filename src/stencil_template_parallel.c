/*

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
  uint neighbours[4];

  int Niterations;
  int periodic;
  vec2_t S, N;

  int Nsources;
  int Nsources_local;
  vec2_t *Sources_local;
  double energy_per_source;

  plane_t planes[2];
  buffers_t buffers[2]; //old new, each has 4
  buffers_t pointers_to_borders[2];
  int output_energy_stat_perstep;

  /* initialize MPI envrionment */
  {
    int level_obtained;

    // NOTE: change MPI_FUNNELED if appropriate
    //
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &level_obtained);
    if (level_obtained < MPI_THREAD_FUNNELED)
    {
      printf("MPI_thread level obtained is %d instead of %d\n",
             level_obtained, MPI_THREAD_FUNNELED);
      MPI_Finalize();
      exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
    MPI_Comm_size(MPI_COMM_WORLD, &Ntasks);
    MPI_Comm_dup(MPI_COMM_WORLD, &myCOMM_WORLD);
  }

  /* argument checking and setting */
  int ret = initialize(&myCOMM_WORLD, Rank, Ntasks, argc, argv, &S, &N, &periodic, &output_energy_stat_perstep,
                       neighbours, &Niterations,
                       &Nsources, &Nsources_local, &Sources_local, &energy_per_source,
                       &planes[0], &buffers[0], pointers_to_borders);

  if (ret)
  {
    printf("task %d is opting out with termination code %d\n",
           Rank, ret);

    MPI_Finalize();
    return 0;
  }

  int current = OLD;
  double t1 = MPI_Wtime(); /* take wall-clock time */

  //somehow fit this in a cycle
  // important for the future 
  // int i, j, k;
  // #pragma omp parallel private(i,k) means that i and k will be unique for each thread and not shared
  
  #pragma omp parallel
  {
    int myid=omp_get_thread_num();
    
    #pragma omp masked filter(myid%4)
    {

      // #define _x_ 0
      // #define _y_ 1

      // #define NORTH 0  _x_
      // #define SOUTH 1  _x_
      // #define EAST 2   _y_
      // #define WEST 3   _y_
      //incoming from, so if the buffer sent is south it needs to be put in in north
      MPI_Status status;
      int x_or_y=myid>>1; //0 if 0 or 1 and 1 if 2 or 3
      int source=(myid&2)|(~(myid&1)); //inverts 0 to 1 or 2 to 3 and vice versa
      //check the various pointers of this line
      //current or not current? not sure, i think not current aka next
      MPI_Recv(&buffers[!current][myid], N[x_or_y],MPI_DOUBLE,source,BORDER_MESSAGE_TAG,&myCOMM_WORLD,&status);
    }
    //TODO parallelize injection, but maybe not so worth it
    //do it only IF Nsources >>> nthread
    //something like this
    #pragma omp masked filter(5)
    {
      inject_energy(periodic, Nsources_local, Sources_local, energy_per_source, &planes[current], N);
    }

  }

  for (int iter = 0; iter < Niterations; ++iter)
  {

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

    update_plane(periodic, N, &planes[current], &planes[!current]);

    /* output if needed */
    if (output_energy_stat_perstep)
      output_energy_stat(iter, &planes[!current], (iter + 1) * Nsources * energy_per_source, Rank, &myCOMM_WORLD);

    /* swap plane indexes for the new iteration */
    current = !current;
  }

  t1 = MPI_Wtime() - t1;

  output_energy_stat(-1, &planes[!current], Niterations * Nsources * energy_per_source, Rank, &myCOMM_WORLD);

  memory_release(buffers, planes, pointers_to_borders);

  MPI_Finalize();
  return 0;
}