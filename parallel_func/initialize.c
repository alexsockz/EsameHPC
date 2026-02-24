#include "stencil_template_parallel.h"

int initialize(MPI_Comm *Comm,
               int Me,        // the rank of the calling process
               int Ntasks,    // the total number of MPI ranks
               int argc,      // the argc from command line
               char **argv,   // the argv from command line
               vec2_t *S,     // the size of the plane
               vec2_t *N,     // two-uint array defining the MPI tasks' grid
               int *periodic, // periodic-boundary tag
               int *output_energy_stat,
               int *verbose,
               int *neighbours,  // four-int array that gives back the neighbours of the calling task
               int *Niterations, // how many iterations
               int *Nsources,    // how many heat sources
               int *Nsources_local,
               vec2_t **Sources_local,
               double *energy_per_source, // how much heat per source
               plane_t *planes,
               buffers_t *buffers,
               buffers_t *borders_ptr)
{
  int halt = 0;
  int ret;
  
  optind = 1; //reset index to say "look at the first option"

  // ··································································
  // set deffault values

  (*S)[_x_] = 10000;
  (*S)[_y_] = 10000;
  *periodic = 0;
  *verbose = 0;
  *Nsources = 4;
  *Nsources_local = 0;
  *Sources_local = NULL;
  *Niterations = 1000;
  *energy_per_source = 1.0;

  if (planes == NULL)
  {
    // manage the situation
  }

  planes[OLD].size[0] = 0;
  planes[NEW].size[0] = 0;

  for (int i = 0; i < 4; i++)
    neighbours[i] = MPI_PROC_NULL;

  for (int b = 0; b < 2; b++)
    for (int d = 0; d < 4; d++)
    {
      buffers[b][d] = NULL;
      borders_ptr[b][d] = NULL;
    }
  // ··································································
  // process the commadn line
  //
  while (1)
  {
    int opt;
    while ((opt = getopt(argc, argv, ":hx:y:e:E:n:o:p:v:")) != -1)
    {
      switch (opt)
      {
      default:
      case 'h':

      {
        if (Me == 0)
          printf("\nvalid options are ( values btw [] are the default values ):\n"
                 "-x    x size of the plate [10000]\n"
                 "-y    y size of the plate [10000]\n"
                 "-e    how many energy sources on the plate [4]\n"
                 "-E    how many energy sources on the plate [1.0]\n"
                 "-n    how many iterations [1000]\n"
                 "-p    whether periodic boundaries applies  [0 = false]\n\n");
        halt = 1;
      }
      break;

      case 'x':
        (*S)[_x_] = (uint)atoi(optarg);
        break;

      case 'y':
        (*S)[_y_] = (uint)atoi(optarg);
        break;

      case 'e':
        *Nsources = atoi(optarg);
        break;

      case 'E':
        *energy_per_source = atof(optarg);
        break;

      case 'n':
        *Niterations = atoi(optarg);
        break;

      case 'o':
        *output_energy_stat = (atoi(optarg) > 0);
        break;

      case 'p':
        *periodic = (atoi(optarg) > 0);
        break;

      case 'v':
        *verbose = atoi(optarg);
        break;

      case ':':
        printf("option -%c requires an argument\n", optopt);
        break;

      case '?':
        printf(" -------- help unavailable ----------\n");
        break;
      }
    }

    if (opt == -1)
      break;
  }

  if (halt)
    return 1;

  // ··································································
  /*
   * here we should check for all the parms being meaningful
   *
   */

  // ...

  // ··································································
  /*
   * find a suitable domain decomposition
   * very simple algorithm, you may want to
   * substitute it with a better one
   *
   * the plane Sx x Sy will be solved with a grid
   * of Nx x Ny MPI tasks
   */

  vec2_t Grid;
  double formfactor = ((*S)[_x_] >= (*S)[_y_] ? (double)(*S)[_x_] / (*S)[_y_] : (double)(*S)[_y_] / (*S)[_x_]);
  int dimensions = 2 - (Ntasks <= ((int)formfactor + 1));

  if (dimensions == 1)
  {
    if ((*S)[_x_] >= (*S)[_y_]){
      Grid[_x_] = Ntasks;
      Grid[_y_] = 1;
    }
    else{
      Grid[_x_] = 1;
      Grid[_y_] = Ntasks;
    }
  }
  else
  {
    int Nf;
    uint *factors;
    uint first = 1;
    ret = simple_factorization(Ntasks, &Nf, &factors);
    if (ret==1)
      return 1;

    for (int i = 0; (i < Nf) && ((Ntasks / first) / first > formfactor); i++)
      first *= factors[i];

    if ((*S)[_x_] > (*S)[_y_]){
      Grid[_x_] = Ntasks / first;
      Grid[_y_] = first;
    }
    else{
      Grid[_x_] = first;
      Grid[_y_] = Ntasks / first;
    }
  }

  (*N)[_x_] = Grid[_x_];
  (*N)[_y_] = Grid[_y_];

  // ··································································
  // my cooridnates in the grid of processors
  //
  int X = Me % Grid[_x_];
  int Y = Me / Grid[_x_];

  // ··································································
  // find my neighbours
  //

  if (Grid[_x_] > 1)
  {
    if (*periodic)
    {
      neighbours[EAST] = Y * Grid[_x_] + ((uint)Me + 1) % Grid[_x_];
      neighbours[WEST] = (X % Grid[_x_] > 0 ? (uint)Me - 1 : (Y + 1) * Grid[_x_] - 1);
    }

    else
    {
      neighbours[EAST] = (X < Grid[_x_] - 1 ? Me + 1 : MPI_PROC_NULL);
      neighbours[WEST] = (X > 0 ? (Me - 1) % Ntasks : MPI_PROC_NULL);
    }
  }

  if (Grid[_y_] > 1)
  {
    if (*periodic)
    {
      neighbours[NORTH] = (Ntasks + Me - Grid[_x_]) % Ntasks;
      neighbours[SOUTH] = (Ntasks + Me + Grid[_x_]) % Ntasks;
    }

    else
    {
      neighbours[NORTH] = (Y > 0 ? Me - Grid[_x_] : MPI_PROC_NULL);
      neighbours[SOUTH] = (Y < Grid[_y_] - 1 ? Me + Grid[_x_] : MPI_PROC_NULL);
    }
  }

  // ··································································
  // the size of my patch
  //

  /*
   * every MPI task determines the size sx x sy of its own domain
   * REMIND: the computational domain will be embedded into a frame
   *         that is (sx+2) x (sy+2)
   *         the outern frame will be used for halo communication or
   */
  vec2_t mysize;
  uint s = (*S)[_x_] / Grid[_x_];
  uint r = (*S)[_x_] % Grid[_x_];
  mysize[_x_] = s + (X < r);
  s = (*S)[_y_] / Grid[_y_];
  r = (*S)[_y_] % Grid[_y_];
  mysize[_y_] = s + (Y < r);

  planes[OLD].size[0] = mysize[0];
  planes[OLD].size[1] = mysize[1];
  planes[NEW].size[0] = mysize[0];
  planes[NEW].size[1] = mysize[1];

  if (*verbose > 0)
  {
    if (Me == 0)
    {
      printf("Tasks are decomposed in a grid %d x %d\n\n",
             Grid[_x_], Grid[_y_]);
      fflush(stdout);
    }

    MPI_Barrier(*Comm);

    for (int t = 0; t < Ntasks; t++)
    {
      if (t == Me)
      {
         printf("Task %4d :: "
           "\tgrid coordinates : %3d, %3d\n"
           "\tneighbours: N %4d    E %4d    S %4d    W %4d\n",
           Me, X, Y,
           (int)neighbours[NORTH], (int)neighbours[EAST],
           (int)neighbours[SOUTH], (int)neighbours[WEST]);
        fflush(stdout);

        /* removed stray number print to avoid interleaved, buffered output */
      }
      
      
      MPI_Barrier(*Comm);
      
    }
    
  }

  // ··································································
  // allocae the needed memory
  //
  if (*verbose) {
      printf("TASK %d: preallocation\n", Me);
    fflush(stdout);
  }
  ret = memory_allocate(neighbours, buffers, borders_ptr, planes);
  if (ret==1)
    return 1;

  // ··································································
  // allocae the heat sources
  //
  ret = initialize_sources(Me, Ntasks, Comm, mysize, *Nsources, Nsources_local, Sources_local);
  if (ret==1)
    return 1;
    
  if (*verbose) {
    printf("TASK %d: finished allocation\n", Me);
    fflush(stdout);
  }
  return 0;
}