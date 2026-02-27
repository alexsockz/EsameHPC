#include "stencil_template_parallel.h"

int get_total_energy(const plane_t * plane,
                            double *energy)
/*
 * NOTE: this routine a good candiadate for openmp
 *       parallelization
 */
{

    const int register xsize = plane->size[_x_];
    const int register ysize = plane->size[_y_];

    double *restrict data = plane->data;

    /* parallel memory layout allocates rows of length xsize and
       adds halo rows in y (ysize + 2). Interior element (i,j) with
       i in [1..xsize], j in [1..ysize] is at index j*xsize + (i-1) */
#define IDX(i, j) ((j) * xsize + ((i)-1))

#if defined(LONG_ACCURACY)
    long double totenergy = 0;
#else
    double totenergy = 0;
#endif

    // HINT: you may attempt to
    //       (i)  manually unroll the loop
    //       (ii) ask the compiler to do it
    // for instance
    // #pragma GCC unroll 4
    for (int j = 1; j <= ysize; j++)
        for (int i = 1; i <= xsize; i++)
            totenergy += data[IDX(i, j)];

#undef IDX

    *energy = (double)totenergy;
    return 0;
}
