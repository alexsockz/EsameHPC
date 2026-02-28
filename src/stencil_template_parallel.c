//#define VERBOSE
//#define MATRIX
//#define OUTPUTENERGY

#include "stencil_template_parallel.h"
#include <stdatomic.h>
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
  vec2_t S;
  vec2_t N;
  vec2_t decomposedS;

  int Nsources;
  int Nsources_local;
  vec2_t *Sources_local;
  double energy_per_source;

  plane_t planes[2];
  buffers_t buffers[2]; // old new, each has 4
  buffers_t border_ptr[2];
  int output_energy_stat_perstep=0;

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
      fflush(stdout);
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
  decomposedS[_x_]=planes[OLD].size[_x_];
  decomposedS[_y_]=planes[OLD].size[_y_];
  int current = OLD;
  double t1 = MPI_Wtime(); /* take wall-clock time */
  double t_tot_inj=0;
  double t_tot_calc= 0;
  double t_tot_send=0;
  // somehow fit this in a cycle
  //  important for the future
  //  int i, j, k;
  //  #pragma omp parallel private(i,k) means that i and k will be unique for each thread and not shared
  
  MPI_Request reqs[8];
  for (int iter = 0; iter < Niterations; ++iter)
  {
    #ifdef VERBOSE
      printf("TASK%d: beforeOMP %d\n", Rank,S[0]);
      fflush(stdout);
    #endif
    
    for (int ri = 0; ri < 8; ++ri) reqs[ri] = MPI_REQUEST_NULL;
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

#ifdef VERBOSE
          printf("TASK%d: thread %d: injecting\n", Rank, myid);
          fflush(stdout);
#endif

        double t_start_inj_local = MPI_Wtime();

        ret = inject_energy(periodic, Nsources_local, Sources_local, energy_per_source, &planes[current], N);

        double t_elapsed_inj = MPI_Wtime() - t_start_inj_local;
        t_tot_inj += t_elapsed_inj;

    /* New parallel pattern:
       - one thread posts the Irecv
       - worker threads compute the border buffers
       - single thread issues non-blocking Isend for all directions (only this thread calls MPI send)
       - worker threads (and the master) run update_plane concurrently
       - master waits for all requests and frees per-iteration allocated buffers
    */

    double *send_buffers[4];
    int send_counts[4];
    atomic_int ready[4];
    for (int si = 0; si < 4; ++si) {
      send_buffers[si] = NULL;
      send_counts[si] = decomposedS[si >> 1];
    }

    /* per-iteration compute timer (shared across threads in the parallel region) */
    double t_start_calc_iter = 0.0;
    #pragma omp parallel
    {
      /* single: post all the Irecv operations (only one thread makes MPI calls)
         this pins MPI usage to a single thread (MPI_THREAD_FUNNELED safe)
      */
      #pragma omp single nowait
      {
        for (int i = 0; i < 4; ++i)
        {
          int x_or_y = i >> 1;
          MPI_Irecv(buffers[!current][i], decomposedS[x_or_y], MPI_DOUBLE, neighbours[i], iter, myCOMM_WORLD, &reqs[4 + i]);
        }

        t_start_calc_iter = MPI_Wtime();

      for (int i = 0; i < 4; ++i)
      {
        int x_or_y = i >> 1;
        double const *old_border = border_ptr[current][i];
        double const *old_buffer = buffers[current][i];
        double *new_border = border_ptr[!current][i];
        double *momentary_buffer = NULL;

        if (x_or_y)
        {
          momentary_buffer = (double *)malloc(decomposedS[_y_] * sizeof(double));
          send_buffers[i] = momentary_buffer;
        }
        else
        {
          /* for the other directions we can use the new_border buffer directly */
          send_buffers[i] = new_border;
        }

        update_border_calc(i, decomposedS, old_border, old_buffer, new_border, momentary_buffer);

        /* mark this direction as ready for sending */
        atomic_store_explicit(&ready[i], 1, memory_order_release);
        MPI_Isend(send_buffers[i], decomposedS[x_or_y], MPI_DOUBLE, neighbours[i], iter, myCOMM_WORLD, &reqs[i]);
      }
      }

      /* compute the inner points in parallel; `update_plane` contains the
         appropriate OpenMP `for` pragma so calling it here will distribute work
         across available threads */
      update_plane(periodic, N, &planes[current], &planes[!current]);

      /* single thread: account compute time, wait for outstanding
         requests (sends + recvs) and free any per-iteration allocated send buffers */
      #pragma omp single
      {
        /* computation time for this iteration: borders + inner plane */
        double t_elapsed_calc = MPI_Wtime() - t_start_calc_iter;
        t_tot_calc += t_elapsed_calc;

        /* measure actual communication waiting time (overlap excluded)
           by timing the Waitall that ensures completion of sends/recvs */
        double t_start_comm_local = MPI_Wtime();
        MPI_Waitall(8, reqs, MPI_STATUS_IGNORE);
        double t_elapsed_comm = MPI_Wtime() - t_start_comm_local;
        t_tot_send += t_elapsed_comm;

        for (int i = 0; i < 4; ++i)
        {
          int x_or_y = i >> 1;
          if (x_or_y && send_buffers[i] != NULL)
          {
            free(send_buffers[i]);
            send_buffers[i] = NULL;
          }
        }
      }
    }
    current = !current;
    /* output if needed */
        /* output if needed */
#ifdef OUTPUTENERGY
    if (output_energy_stat_perstep)
    {
        output_energy_stat(iter, &planes[!current], (iter + 1) * Nsources * energy_per_source, Rank, &myCOMM_WORLD);
#ifdef MATRIX
        print_matrix(Rank, Ntasks, planes[!current].size[_x_], planes[!current].size[_y_], planes[!current].data, buffers[!current], myCOMM_WORLD);
#endif

    }
#endif
    /* swap plane indexes for the new iteration */
  }

  t1 = MPI_Wtime() - t1;


      /* ensure any outstanding non-blocking sends complete before next step */
      MPI_Waitall(8, reqs, MPI_STATUS_IGNORE);
  
    double total_time_mean, computation_time_mean, communication_time_mean, energy_injection_time_mean;

 // consider the mean for each time variable across all tasks
  MPI_Reduce(&t1, &total_time_mean, 1, MPI_DOUBLE, MPI_SUM, 0, myCOMM_WORLD);
  MPI_Reduce(&t_tot_calc, &computation_time_mean, 1, MPI_DOUBLE, MPI_SUM, 0, myCOMM_WORLD);
  MPI_Reduce(&t_tot_send, &communication_time_mean, 1, MPI_DOUBLE, MPI_SUM, 0, myCOMM_WORLD);
  //MPI_Reduce(&t_tot_, &waiting_time_mean, 1, MPI_DOUBLE, MPI_SUM, 0, myCOMM_WORLD);
  MPI_Reduce(&t_tot_inj, &energy_injection_time_mean, 1, MPI_DOUBLE, MPI_SUM, 0, myCOMM_WORLD);

  // add the code to print the time to post process them
  if (Rank == 0 || Ntasks == 1) {
    const char *job_name = getenv("JOB_NAME");

    const char *output_dir = "output";
      // Build full path
    char filename[512];
    snprintf(filename, sizeof(filename), "%s/%s.csv", output_dir, job_name);

    FILE *f = fopen(filename, "w");
      if (!f) {
          perror("fopen");
          exit(1);
      }

      fprintf(f,"Mean_total_time,Mean_Computation_time,Mean_Communication_time,energy_injection_time_mean,Grid_x,Grid_y,N_iterations\n");
      fprintf(f,"%f,%f,%f,%f,%u,%u,%d\n",
              total_time_mean/Ntasks,
              computation_time_mean/Ntasks,
              communication_time_mean/Ntasks,
              energy_injection_time_mean/Ntasks,
              S[_x_],
              S[_y_],
              Niterations);
        fflush(f);
      fclose(f);

    
  }

  output_energy_stat(-1, &planes[!current], Niterations * Nsources * energy_per_source, Rank, &myCOMM_WORLD);
  printf("PROCESS %d: time taken %f, computing %f, communicating %f, injecting %f \n", Rank, t1, t_tot_calc, t_tot_send, t_tot_inj);
  fflush(stdout);
  memory_release(planes, buffers, border_ptr);

  MPI_Finalize();
  return 0;
}