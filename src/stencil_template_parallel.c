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
  vec2_t S, N, decomposedS;

  int Nsources;
  int Nsources_local;
  vec2_t *Sources_local;
  double energy_per_source;

  plane_t planes[2];
  buffers_t buffers[2]; // old new, each has 4
  buffers_t border_ptr[2];
  int output_energy_stat_perstep;

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
  decomposedS[_x_]=planes[OLD].size[_x_];
  decomposedS[_y_]=planes[OLD].size[_y_];
  int current = OLD;
  double t1 = MPI_Wtime(); /* take wall-clock time */
  double t_tot_inj=0;
  double t_start_inj;
  double t_tot_calc= 0;
  double t_start_calc;
  double t_tot_send=0;
  double t_start_send;
  // somehow fit this in a cycle
  //  important for the future
  //  int i, j, k;
  //  #pragma omp parallel private(i,k) means that i and k will be unique for each thread and not shared

  for (int iter = 0; iter < Niterations; ++iter)
  {
    bool injected = false;

    #ifdef VERBOSE
      printf("TASK%d: beforeOMP %d\n", Rank,S[0]);
      fflush(stdout);
    #endif
    
    MPI_Request reqs[8];

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
        // check the various pointers of this line
        // current or not current? not sure, i think not current aka next
        //  maybe change tag to current iteration number
#ifdef VERBOSE
          printf("TASK%d: thread %d: waiting for Mpi_recv from %d, source %d \n", Rank, myid, neighbours[myid],myid);
          fflush(stdout);
#endif

        MPI_Recv(buffers[!current][myid], decomposedS[x_or_y], MPI_DOUBLE, neighbours[myid], iter, myCOMM_WORLD, &status);

#ifdef VERBOSE
          printf("TASK%d: thread %d: recieved from %d\n", Rank, myid, neighbours[myid]);
          fflush(stdout);
#endif        
      }
      // TODO parallelize injection, but maybe not so worth it
      // do it only IF Nsources >>> nthread

#pragma omp masked filter(8)
      {
#ifdef VERBOSE
          printf("TASK%d: thread %d: injecting\n", Rank, myid);
          fflush(stdout);
#endif

        t_start_inj=MPI_Wtime();
        ret = inject_energy(periodic, Nsources_local, Sources_local, energy_per_source, &planes[current], N);
        if (ret == 0)
        {
          #pragma omp atomic write
          injected = true;
          #pragma omp flush(injected)
        }
        t_tot_inj+=(MPI_Wtime()-t_start_inj);
#ifdef VERBOSE
          printf("TASK%d: thread %d: injected\n", Rank, myid);
          printf("TASK%d: thread %d: updating plane\n", Rank, myid);
          fflush(stdout);
#endif

        t_start_calc=MPI_Wtime();
        update_plane(periodic, N, &planes[current], &planes[!current]);
#ifdef VERBOSE
          printf("TASK%d: thread %d: updated plane\n", Rank, myid);
          fflush(stdout);
#endif

        t_tot_calc+=(MPI_Wtime()-t_start_calc);
      }

      if (myid > 3 && myid < 8)
      {
        register double alpha = ALPHA;
        register double alpha_inverse = 1 / 4.0 * (1 - alpha);
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

        t_start_send=MPI_Wtime();
        int work_direction = myid - 4;
#ifdef VERBOSE
          printf("TASK%d: thread %d: calculating border %d\n", Rank, myid, work_direction);
          fflush(stdout);
#endif
        int x_or_y = work_direction >> 1; // 0 if 0 or 1 and 1 if 2 or 3
        double const *old_border = border_ptr[current][work_direction];
        double const *old_buffer = buffers[current][work_direction];
        double *new_border = border_ptr[!current][work_direction];
        int next_row = decomposedS[_x_];
        if (x_or_y)
        {
          //vertical: WEST EAST
          int skips = 0;
          int plus_or_minus_one = -1;
          // TODO either do this or switch to a personalized MPI_TYPE
          double *momentary_buffer = (double *)malloc(decomposedS[_y_] * sizeof(double));

          if (work_direction == WEST)
            plus_or_minus_one = 1;
          for (uint i = 0; i < decomposedS[x_or_y]; i++)
          {
            double result = old_border[skips] * alpha;
            // perpendicular to direction
            double sum_i = (old_buffer[i] + old_border[skips + plus_or_minus_one]) * alpha_inverse;
            // parallel
            // might be illegal
            double sum_j = (old_border[skips - next_row] + old_border[skips + next_row]) * alpha_inverse;
            result += (sum_i + sum_j);
            #pragma omp atomic write
            new_border[skips] = result;
            #pragma omp flush(new_border)
            momentary_buffer[i] = result;

            skips += next_row;
          }
#ifdef VERBOSE
            printf("TASK%d: thread %d: calculated border %d\n", Rank, myid, work_direction);
            fflush(stdout);
#endif
          
          MPI_Send(momentary_buffer, decomposedS[x_or_y], MPI_DOUBLE, neighbours[work_direction], iter, myCOMM_WORLD);
          free(momentary_buffer);

#ifdef VERBOSE
            printf("TASK%d: thread %d: sent border to %d, direction %d\n", Rank, myid, neighbours[work_direction], work_direction);
            fflush(stdout);
#endif
        }
        else
        {
          for (uint i = 1; i < decomposedS[x_or_y] - 1; i++)
          {
            double result = old_border[i] * alpha;
            // parallel to dircetion
            double sum_i = (old_border[i - 1] + old_border[i + 1]) * alpha_inverse;
            // perpendicular
            double sum_j = (old_buffer[i] + old_border[i + next_row]) * alpha_inverse;
            result += (sum_i + sum_j);
            #pragma omp atomic write
            new_border[i] = result;
            #pragma omp flush(new_border)
          }
#ifdef VERBOSE
            printf("TASK%d: thread %d: calculated border\n", Rank, myid);
            fflush(stdout);
#endif
            MPI_Isend(new_border, decomposedS[x_or_y], MPI_DOUBLE, neighbours[work_direction], iter, myCOMM_WORLD, &reqs[work_direction]);
#ifdef VERBOSE
            printf("TASK%d: thread %d: sent border to %d, direction %d\n", Rank, myid, neighbours[work_direction], work_direction);
            fflush(stdout);
#endif
        }
        t_tot_send+=(MPI_Wtime()-t_start_send);
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
      fclose(f);

    
  }

  output_energy_stat(-1, &planes[!current], Niterations * Nsources * energy_per_source, Rank, &myCOMM_WORLD);
  printf("time taken %f\n", t1);
  memory_release(planes, buffers, border_ptr);

  MPI_Finalize();
  return 0;
}