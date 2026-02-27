#include "stencil_template_parallel.h"

void update_border_calc(int work_direction,
                        const vec2_t decomposedS,
                        double const *old_border,
                        double const *old_buffer,
                        double *new_border,
                        double *momentary_buffer)
{
    register double alpha = ALPHA;
    register double alpha_inverse = 1 / 4.0 * (1 - alpha);

    int x_or_y2 = work_direction >> 1; // 0 for horizontal, 1 for vertical
    int next_row2 = decomposedS[_x_];

    if (x_or_y2)
    {
        int skips = 0;
        int plus_or_minus_one = -1;
        if (work_direction == WEST)
            plus_or_minus_one = 1;

        for (uint i = 0; i < decomposedS[x_or_y2]; i++)
        {
            double result = old_border[skips] * alpha;
            double sum_i = (old_buffer[i] + old_border[skips + plus_or_minus_one]) * alpha_inverse;
            double sum_j = (old_border[skips - next_row2] + old_border[skips + next_row2]) * alpha_inverse;
            result += (sum_i + sum_j);
            #pragma omp atomic write
            new_border[skips] = result;
            #pragma omp flush(new_border)
            if (momentary_buffer)
                momentary_buffer[i] = result;

            skips += next_row2;
        }
    }
    else
    {
        for (uint i = 1; i < decomposedS[x_or_y2] - 1; i++)
        {
            double result = old_border[i] * alpha;
            double sum_i = (old_border[i - 1] + old_border[i + 1]) * alpha_inverse;
            double sum_j = (old_buffer[i] + old_border[i + next_row2]) * alpha_inverse;
            result += (sum_i + sum_j);
            #pragma omp atomic write
            new_border[i] = result;
            #pragma omp flush(new_border)
        }
    }
}
