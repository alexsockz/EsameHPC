#include "stencil_template_parallel.h"

int memory_release(plane_t *planes, buffers_t * buffers_ptr)

{

  if (planes != NULL)
  {
    if (planes[OLD].data != NULL)
      free(planes[OLD].data);

    if (planes[NEW].data != NULL)
      free(planes[NEW].data);
  }
  if(buffers_ptr!=NULL && buffers_ptr[OLD][EAST]!=NULL)  {
    buffers_ptr[OLD][NORTH]=NULL;
    buffers_ptr[OLD][SOUTH]=NULL;
    buffers_ptr[NEW][NORTH]=NULL;
    buffers_ptr[NEW][SOUTH]=NULL;
  //allocating 2 buffers for east and west 
    free(buffers_ptr[OLD][EAST]);
    free(buffers_ptr[OLD][WEST]);
    free(buffers_ptr[NEW][EAST]);
    free(buffers_ptr[NEW][WEST]);
  }
  

  return 0;
}