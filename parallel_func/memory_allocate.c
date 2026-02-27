#include "stencil_template_parallel.h"

int memory_allocate(buffers_t *buffers_ptr,
                    buffers_t *borders_ptr,
                    plane_t *planes_ptr)

{
  /*
    here you allocate the memory buffers that you need to
    (i)  hold the results of your computation
    (ii) communicate with your neighbours

    The memory layout that I propose to you is as follows:

    (i) --- calculations
    you need 2 memory regions: the "OLD" one that contains the
    results for the step (i-1)th, and the "NEW" one that will contain
    the updated results from the step ith.

    Then, the "NEW" will be treated as "OLD" and viceversa.

    These two memory regions are indexed by *plate_ptr:

    planew_ptr[0] ==> the "OLD" region
    plamew_ptr[1] ==> the "NEW" region


    (ii) --- communications

    you may need two buffers (one for sending and one for receiving)
    for each one of your neighnours, that are at most 4:
    north, south, east amd west.

    To them you need to communicate at most mysizex or mysizey
    daouble data.

    These buffers are indexed by the buffer_ptr pointer so
    that

    (*buffers_ptr)[SEND][ {NORTH,...,WEST} ] = .. some memory regions
    (*buffers_ptr)[RECV][ {NORTH,...,WEST} ] = .. some memory regions

    --->> Of course you can change this layout as you prefer

   */
  if (planes_ptr == NULL)
    return 1;

  if (buffers_ptr == NULL)
    return 1;

  // ··················································
  // allocate memory for data
  // we allocate the space needed for the plane plus a contour frame
  // that will contains data form neighbouring MPI tasks
  int x_size= planes_ptr[OLD].size[_x_];
  int y_size= planes_ptr[OLD].size[_y_];
  unsigned int frame_size = x_size * (y_size+2);

  planes_ptr[OLD].data = (double *)malloc(frame_size * sizeof(double));
  if (planes_ptr[OLD].data == NULL)
    return 1;
  else
    memset(planes_ptr[OLD].data, 0, frame_size * sizeof(double));

  planes_ptr[NEW].data = (double *)malloc(frame_size * sizeof(double));
  if (planes_ptr[NEW].data == NULL)
    return 1;
  else
    memset(planes_ptr[NEW].data, 0, frame_size * sizeof(double));
  
  // ··················································
  // buffers for north and south communication
  // are not really needed
  //
  // in fact, they are already contiguous, just the
  // first and last line of every rank's plane
  //
  // you may just make some pointers pointing to the
  // correct positions
  //

  // or, if you preer, just go on and allocate buffers
  // also for north and south communications

  // ··················································
  // allocate buffers
  //
  // ··················································

  //DOING ALL OF THIS EXPLICITLY TO REMEMBER WHAT IT MEANS
  //pointing to north and south for

  buffers_ptr[OLD][NORTH] = &(planes_ptr[OLD].data[0]);
  buffers_ptr[OLD][SOUTH] = &(planes_ptr[OLD].data[(y_size + 1) * x_size]);
  buffers_ptr[NEW][NORTH] = &(planes_ptr[NEW].data[0]);
  buffers_ptr[NEW][SOUTH] = &(planes_ptr[NEW].data[(y_size + 1) * x_size]);

  //allocating 2 buffers for east and west 
  buffers_ptr[OLD][WEST] = (double *)malloc(y_size * sizeof(double));
  if (buffers_ptr[OLD][WEST] != NULL)
    memset(buffers_ptr[OLD][WEST], 0, y_size * sizeof(double));
  buffers_ptr[OLD][EAST] = (double *)malloc(y_size * sizeof(double));
  if (buffers_ptr[OLD][EAST] != NULL)
    memset(buffers_ptr[OLD][EAST], 0, y_size * sizeof(double));
  buffers_ptr[NEW][WEST] = (double *)malloc(y_size * sizeof(double));
  if (buffers_ptr[NEW][WEST] != NULL)
    memset(buffers_ptr[NEW][WEST], 0, y_size * sizeof(double));
  buffers_ptr[NEW][EAST] = (double *)malloc(y_size * sizeof(double));
  if (buffers_ptr[NEW][EAST] != NULL)
    memset(buffers_ptr[NEW][EAST], 0, y_size * sizeof(double));

  // ··················································
  // pointers to borders to make modification of just them easier
  //
  // ··················································
  for(int t=0; t<2; ++t)
  {
    //pointers to EAST and NORTH are the same because i can't force c to accept that 
    //by summing 1 to a pointer i actually want it to shift by _x_
    //so for now i just point to such a place in the matrix
    //in the future i will have 2 matrices, one row major and one column major
        borders_ptr[t][NORTH] = &(planes_ptr[t].data[1 * x_size + 0]);
        borders_ptr[t][SOUTH] = &(planes_ptr[t].data[y_size * x_size]);
        borders_ptr[t][WEST] = &planes_ptr[t].data[x_size];
        borders_ptr[t][EAST] = &planes_ptr[t].data[(2 * x_size) - 1];
  }
  return 0;
}