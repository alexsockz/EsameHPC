#include "stencil_template_parallel.h"

static void release_buffer_slot(buffers_t *buffers_ptr, int t, int d)
{
  /* NORTH and SOUTH point into the plane data (no malloc), don't free them */
  if (d == NORTH || d == SOUTH)
  {
    buffers_ptr[t][d] = NULL;
  }
  else if (buffers_ptr[t][d] != NULL)
  {
    free(buffers_ptr[t][d]);
    buffers_ptr[t][d] = NULL;
  }
}

int memory_release(plane_t *planes, buffers_t * buffers_ptr, buffers_t * border_ptr)
{

  if (planes != NULL)
  {
    if (planes[OLD].data != NULL)
    {
      free(planes[OLD].data);
      planes[OLD].data = NULL;
    }

    if (planes[NEW].data != NULL)
    {
      free(planes[NEW].data);
      planes[NEW].data = NULL;
    }
  }

  if (buffers_ptr != NULL)
  {
    for (int t = 0; t < 2; ++t)
    {
      for (int d = 0; d < 4; ++d)
      {
        release_buffer_slot(buffers_ptr, t, d);
      }
    }
  }

  if (border_ptr != NULL)
  {
    for (int t = 0; t < 2; ++t)
    {
      for (int b = 0; b < 4; ++b)
      {
        border_ptr[t][b] = NULL;
      }
    }
  }

  return 0;
}