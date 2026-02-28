//#define VERBOSE
//#define MATRIX

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
    bool injected = false;

    #ifdef VERBOSE
      printf("TASK%d: beforeOMP %d\n", Rank,S[0]);
      fflush(stdout);
    #endif
    
    for (int ri = 0; ri < 8; ++ri) reqs[ri] = MPI_REQUEST_NULL;

    fflush(stdout);

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

#pragma omp parallel
    {
#pragma omp masked
      {
#ifdef VERBOSE
          printf("TASK%d: thread %d: injecting\n", Rank, myid);
          fflush(stdout);
#endif

        double t_start_inj_local = MPI_Wtime();
        ret = inject_energy(periodic, Nsources_local, Sources_local, energy_per_source, &planes[current], N);
        if (ret == 0)
        {
          #pragma omp atomic write
          injected = true;
          #pragma omp flush(injected)
        }
        double t_elapsed_inj = MPI_Wtime() - t_start_inj_local;
        #pragma omp atomic
        t_tot_inj += t_elapsed_inj;
#ifdef VERBOSE
          printf("TASK%d: thread %d: injected\n", Rank, myid);
          printf("TASK%d: thread %d: updating plane\n", Rank, myid);
          fflush(stdout);
#endif

        double t_start_calc_local = MPI_Wtime();
        update_plane(periodic, N, &planes[current], &planes[!current]);
      #ifdef VERBOSE
          printf("TASK%d: thread %d: updated plane\n", Rank, myid);
          fflush(stdout);
      #endif

        double t_elapsed_calc = MPI_Wtime() - t_start_calc_local;
        #pragma omp atomic
        t_tot_calc += t_elapsed_calc;
      }
              /* For the 4 communication directions use a small parallel region that
         provides each worker with a private `i` in 0..3. This makes the send
         thread-agnostic while keeping the original logic and injection
         synchronization intact. */
        int i;
        #pragma omp for schedule(dynamic) private(i)
        for (i = 0; i < 4; i++)
        {
          MPI_Status status;
          int x_or_y = i >> 1;                     // 0 if 0 or 1 and 1 if 2 or 3

#ifdef VERBOSE
          printf("TASK%d: comm-thread %d: posting Irecv from %d\n", Rank, i, neighbours[i]);
          fflush(stdout);
#endif
          MPI_Irecv(buffers[!current][i], decomposedS[x_or_y], MPI_DOUBLE, neighbours[i], iter, myCOMM_WORLD, &reqs[4 + i]);

          /* wait for injection to complete (preserve original behavior) */
          int val = 0;
          while (!val)
          {
#pragma omp atomic read
            val = injected;
#pragma omp flush(injected)
          }

          double t_start_send_local = MPI_Wtime();
#ifdef VERBOSE
            printf("TASK%d: comm-thread %d: calculating border %d\n", Rank, i, i);
            fflush(stdout);
#endif
          double const *old_border = border_ptr[current][i];
          double const *old_buffer = buffers[current][i];
          double *new_border = border_ptr[!current][i];

          double *momentary_buffer = NULL;
          if (x_or_y)
          {
            momentary_buffer = (double *)malloc(decomposedS[_y_] * sizeof(double));
          }

          update_border_calc(i, decomposedS, old_border, old_buffer, new_border, momentary_buffer);

          if (x_or_y)
          {
            MPI_Isend(momentary_buffer, decomposedS[x_or_y], MPI_DOUBLE, neighbours[i], iter, myCOMM_WORLD, &reqs[i]);

            double t_elapsed_send = MPI_Wtime() - t_start_send_local;
            #pragma omp atomic
            t_tot_send += t_elapsed_send;

            if (reqs[i] != MPI_REQUEST_NULL)
            {
              MPI_Wait(&reqs[i], MPI_STATUS_IGNORE);
              reqs[i] = MPI_REQUEST_NULL;
            }
            free(momentary_buffer);
          }
          else
          {
            MPI_Isend(new_border, decomposedS[x_or_y], MPI_DOUBLE, neighbours[i], iter, myCOMM_WORLD, &reqs[i]);

            double t_elapsed_send = MPI_Wtime() - t_start_send_local;
            #pragma omp atomic
            t_tot_send += t_elapsed_send;

            if (reqs[i] != MPI_REQUEST_NULL)
            {
              MPI_Wait(&reqs[i], MPI_STATUS_IGNORE);
              reqs[i] = MPI_REQUEST_NULL;
            }
          }

          /* ensure the posted Irecv completes before exiting this worker */
          MPI_Wait(&reqs[4 + i], &status);
        }
      }
    /* output if needed */
        /* output if needed */
    if (output_energy_stat_perstep)
    {
        output_energy_stat(iter, &planes[!current], (iter + 1) * Nsources * energy_per_source, Rank, &myCOMM_WORLD);
#ifdef MATRIX
        print_matrix(Rank, Ntasks, planes[!current].size[_x_], planes[!current].size[_y_], planes[!current].data, buffers[!current], myCOMM_WORLD);
#endif

    }
    /* swap plane indexes for the new iteration */
    current = !current;
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