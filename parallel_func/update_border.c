#include "stencil_template_parallel.h"

//TODO BIG TODO THIS IS EASELY GENERALIZABLE AND REFACTORIZABLE FOR GENERAL USE
// move mpi call outside
// è tutto generalizzabile ad un solo ciclo se trasformo il for in while

int update_border(int myid, int iter, double const *old_border,double const *old_buffer, 
                    double *new_border, const vec2_t S, const int* neighbours,MPI_Comm *Comm, MPI_Request* reqs)
{
    double const alpha = ALPHA;
    double const alpha_inverse=1/ 4.0 * (1 - alpha);
    
    int work_direction = myid - 4;
#ifdef DEBUG
    printf("TASK%d: thread %d: calculating border %d\n", Rank, myid, work_direction);
    fflush(stdout);
#endif
    int x_or_y = work_direction >> 1; // 0 if 0 or 1 and 1 if 2 or 3
    //double const *old_border = border_ptr[current][work_direction];
    //double const *old_buffer = buffers[current][work_direction];
    //double *new_border = border_ptr[!current][work_direction];
    int next_row = S[_x_];
    if (x_or_y)
    {
        int skips = 0;
        int plus_or_minus_one = work_direction == WEST ? 1 : -1;
        // TODO either do this or switch to a personalized MPI_TYPE
        double *momentary_buffer = (double *)malloc(S[_y_] * sizeof(double));

        for (uint i = 0; i < S[x_or_y]; i++)
        {
            double result = old_border[skips] * alpha;
            // perpendicular to direction
            double sum_i = (old_buffer[i] + old_border[skips + plus_or_minus_one]) * alpha_inverse;
            // parallel
            // might be illegal
            double sum_j = (old_border[skips - next_row] + old_border[skips + next_row]) * alpha_inverse;
            result += (sum_i + sum_j);
            new_border[skips] = result;
            momentary_buffer[i] = result;

            skips += next_row;
        }
#ifdef DEBUG
        printf("TASK%d: thread %d: calculated border %d\n", Rank, myid, work_direction);
        fflush(stdout);
#endif
        MPI_Send(momentary_buffer, S[x_or_y], MPI_DOUBLE, neighbours[work_direction], iter, Comm);
        free(momentary_buffer);
#ifdef DEBUG
        printf("TASK%d: thread %d: sent border to %d, direction %d\n", Rank, myid, neighbours[work_direction], work_direction);
        fflush(stdout);
#endif
    }
    else
    {
        for (uint i = 1; i < S[x_or_y] - 1; i++)
        {
            double result = old_border[i] * alpha;
            // parallel to dircetion
            double sum_i = (old_border[i - 1] + old_border[i + 1]) * alpha_inverse;
            // perpendicular
            double sum_j = (old_buffer[i] + old_border[i + next_row]) * alpha_inverse;
            result += (sum_i + sum_j);
            new_border[i] = result;
        }
#ifdef DEBUG
        printf("TASK%d: thread %d: calculated border\n", Rank, myid);
        fflush(stdout);
#endif
        MPI_Isend(new_border, S[x_or_y], MPI_DOUBLE, neighbours[work_direction], iter, Comm,
                  &reqs[work_direction]);

#ifdef DEBUG
        printf("TASK%d: thread %d: sent border to %d, direction %d\n",
               Rank, myid, neighbours[work_direction], work_direction);
        fflush(stdout);
#endif
    }
    return 0;
}
