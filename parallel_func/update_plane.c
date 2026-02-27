#include "stencil_template_parallel.h"

int update_plane(const int periodic,
                        const vec2_t N, // the grid of MPI tasks
                        const plane_t *oldplane,
                        plane_t *newplane)
{
    uint register fxsize = oldplane->size[_x_];

    uint register xsize = oldplane->size[_x_];
    uint register ysize = oldplane->size[_y_];

#define IDX(i, j) ((j) * fxsize + (i))

    // HINT: you may attempt to
    //       (i)  manually unroll the loop
    //       (ii) ask the compiler to do it
    // for instance
    // #pragma GCC unroll 4
    //
    // HINT: in any case, this loop is a good candidate
    //       for openmp parallelization

    double *restrict old = oldplane->data;
    double *restrict new = newplane->data;
    double const alpha = ALPHA;
    double const alpha_inverse=1/ 4.0 * (1 - alpha);

    for (uint j = 2; j < ysize-1; j++)//exclude borders and halo
        for (uint i = 1; i < xsize; i++)//exclude borders
        {

            // NOTE: (i-1,j), (i+1,j), (i,j-1) and (i,j+1) always exist even
            //       if this patch is at some border without periodic conditions;
            //       in that case it is assumed that the +-1 points are outside the
            //       plate and always have a value of 0, i.e. they are an
            //       "infinite sink" of heat

            // five-points stencil formula
            //
            // HINT : check the serial version for some optimization
            //
            double result = old[IDX(i, j)] * alpha;
            double sum_i = (old[IDX(i - 1, j)] + old[IDX(i + 1, j)]) *alpha_inverse;
            double sum_j = (old[IDX(i, j - 1)] + old[IDX(i, j + 1)]) *alpha_inverse;
            result += (sum_i + sum_j);
            new[IDX(i, j)] = result;
        }

    if (periodic)
    {
        if (N[_x_] == 1)
        {
            // propagate the boundaries as needed
            // check the serial version
        }

        if (N[_y_] == 1)
        {
            // propagate the boundaries as needed
            // check the serial version
        }
    }

#undef IDX
    return 0;
}