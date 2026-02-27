#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "stencil_template_parallel.h"

int main() {
    // Setup
    plane_t planes[2];
    buffers_t buffers[2];
    buffers_t borders_ptr[2];
    vec2_t N = {4, 4}; // 4x4 grid

    // Initialize plane sizes
    planes[OLD].size[_x_] = N[_x_];
    planes[OLD].size[_y_] = N[_y_];
    planes[NEW].size[_x_] = N[_x_];
    planes[NEW].size[_y_] = N[_y_];

    // Call the function
    int ret = memory_allocate(buffers, borders_ptr, planes);

    // Test allocations
    assert(ret == 0);
    assert(planes[OLD].data != NULL);
    assert(planes[NEW].data != NULL);

    // Test north/south pointers
    assert(buffers[OLD][NORTH] == &planes[OLD].data[0]);
    assert(buffers[OLD][SOUTH] == &planes[OLD].data[(planes[OLD].size[_y_] + 1) * planes[OLD].size[_x_]]);
    assert(buffers[NEW][NORTH] == &planes[NEW].data[0]);
    assert(buffers[NEW][SOUTH] == &planes[NEW].data[(planes[NEW].size[_y_] + 1) * planes[NEW].size[_x_]]);


    planes[OLD].data[5]=1;
    assert(buffers[OLD][NORTH][5]==planes[OLD].data[5]);
    assert(buffers[OLD][NORTH][5]==1);
    // Test east/west allocations
    assert(buffers[OLD][EAST] != NULL);
    assert(buffers[OLD][WEST] != NULL);
    assert(buffers[NEW][EAST] != NULL);
    assert(buffers[NEW][WEST] != NULL);

    // Clean up
    free(planes[OLD].data);
    free(planes[NEW].data);
    free(buffers[OLD][EAST]);
    free(buffers[OLD][WEST]);
    free(buffers[NEW][EAST]);
    free(buffers[NEW][WEST]);

    printf("memory_allocate test passed.\n");
    return 0;
}