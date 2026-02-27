#include "stencil_template_parallel.h"


inline void print_matrix_basic( int x_size, int y_size, const double* matrix_ptr,
                                    const double* west_buf, const double* east_buf)
{
    if (matrix_ptr == NULL || x_size <= 0 || y_size <= 0)
        return;

    /* Layout: matrix_ptr contains (y_size+2) rows of length x_size:
         row 0 = north halo, rows 1..y_size = core, row y_size+1 = south halo
         west_buf/east_buf (if non-NULL) contain y_size values for vertical halos */

    int r, c;
    const double *ptr = matrix_ptr;

    const int indent = 20;
    for (int i = 0; i < indent; ++i) putchar(' ');
    putchar('[');
    for (c = 0; c < x_size; ++c) {
        printf("%0.3f", ptr[c]);
        if (c + 1 < x_size) printf(", ");
    }
    printf("]\n");

    for (r = 0; r < y_size; ++r) {
        /* use provided side buffers if present, otherwise fall back to core edges */
        const double left_halo  = (west_buf != NULL) ? west_buf[r] : ptr[(r + 1) * x_size + 0];
        const double right_halo = (east_buf != NULL) ? east_buf[r] : ptr[(r + 1) * x_size + (x_size - 1)];

        printf("%15.3f", left_halo);

        printf(" [");
        for (c = 0; c < x_size; ++c) {
            printf("%0.3f", ptr[(r + 1) * x_size + c]);
            if (c + 1 < x_size) printf(", ");
        }
        printf("]");

        printf("%15.3f\n", right_halo);
    }

    for (int i = 0; i < indent; ++i) putchar(' ');
    putchar('[');
    for (c = 0; c < x_size; ++c) {
        printf("%0.3f", ptr[(y_size + 1) * x_size + c]);
        if (c + 1 < x_size) printf(", ");
    }
    printf("]\n");

}


inline void print_matrix(int rank, int Ntasks, int x_size, int y_size, const double* matrix_ptr,
                                                                        double *restrict buffer[], MPI_Comm Comm){
        for (int x = 0; x < Ntasks; x++) {
                MPI_Barrier(Comm);
                if (x == rank) {
                        printf("process %d matrix:\n", rank);
                        print_matrix_basic(x_size, y_size, matrix_ptr, buffer[WEST], buffer[EAST]);
                        printf("------------------------------------------------------------------\n");
                        fflush(stdout);
                }
        }
}