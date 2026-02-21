#include "stencil_template_parallel.h"

int memory_allocate(const int *neighbours,
                    buffers_t *buffers_ptr,
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
    // an invalid pointer has been passed
    // manage the situation
    ;

  if (buffers_ptr == NULL)
    // an invalid pointer has been passed
    // manage the situation
    ;

  // ··················································
  // allocate memory for data
  // we allocate the space needed for the plane plus a contour frame
  // that will contains data form neighbouring MPI tasks
  unsigned int frame_size = (planes_ptr[OLD].size[_x_]) * (planes_ptr[OLD].size[_y_]+2);

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
  buffers_ptr[OLD][NORTH]=&(planes_ptr[OLD].data[0]);
  buffers_ptr[OLD][SOUTH]=&(planes_ptr[OLD].data[planes_ptr[OLD].size[_x_]+1]);
  buffers_ptr[NEW][NORTH]=&(planes_ptr[NEW].data[0]);
  buffers_ptr[NEW][SOUTH]=&(planes_ptr[NEW].data[planes_ptr[NEW].size[_x_]+1]);
  //allocating 2 buffers for east and west 
  buffers_ptr[OLD][EAST]=(double *)malloc(planes_ptr[OLD].size[_y_]*sizeof(double));
  buffers_ptr[OLD][WEST]=(double *)malloc(planes_ptr[OLD].size[_y_]*sizeof(double));
  buffers_ptr[NEW][EAST]=(double *)malloc(planes_ptr[NEW].size[_y_]*sizeof(double));
  buffers_ptr[NEW][WEST]=(double *)malloc(planes_ptr[NEW].size[_y_]*sizeof(double));
  return 0;
}