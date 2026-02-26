void print_matrix(int x, int y, double* ptr)
{
    
    for(int j=0; j<y; j++)
    {
        printf("[");
        for (int i=0; i<x; i++)
            {
            printf("%f, ", ptr[i]);
            }
        printf("%f]\n");
    }
}