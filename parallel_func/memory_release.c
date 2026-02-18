#include "stencil_template_parallel.h"

int memory_release(plane_t *planes,
                   ....)

{

  if (planes != NULL)
  {
    if (planes[OLD].data != NULL)
      free(planes[OLD].data);

    if (planes[NEW].data != NULL)
      free(planes[NEW].data);
  }

  return 0;
}