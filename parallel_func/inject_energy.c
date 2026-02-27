#include "stencil_template_parallel.h"

int inject_energy(const int periodic,
                         const int Nsources,
                         const vec2_t *Sources,
                         const double energy,
                         plane_t *plane,
                         const vec2_t N)
{
    const uint register sizex = plane->size[_x_];
    double *restrict data = plane->data;

/* interior x coordinates are in [1..sizex] so map to 0-based storage */
#define IDX(i, j) ((j) * sizex + ((i)-1))
    for (int s = 0; s < Nsources; s++)
    {
        int x = Sources[s][_x_];
        int y = Sources[s][_y_];

        data[IDX(x, y)] += energy;

        if (periodic)
        {
            if ((N[_x_] == 1))
            {
                // propagate the boundaries if needed
                // check the serial version
            }

            if ((N[_y_] == 1))
            {
                // propagate the boundaries if needed
                // check the serial version
            }
        }
    }
#undef IDX

    return 0;
}
