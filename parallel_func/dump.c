#include "stencil_template_parallel.h"
#include <float.h>

int dump(const double *data, const uint size[2], const char *filename, double *min, double *max)
{
  if ((filename != NULL) && (filename[0] != '\0'))
  {
    FILE *outfile = fopen(filename, "w");
    if (outfile == NULL)
      return 2;

    float *array = (float *)malloc(size[0] * sizeof(float));

    double _min_ = DBL_MAX;
    double _max_ = 0;

    for (int j = 0; j < size[1]; j++)
    {
      /*
      float y = (float)j / size[1];
      fwrite ( &y, sizeof(float), 1, outfile );
      */

      const double *restrict line = data + j * size[0];
      for (int i = 0; i < size[0]; i++)
      {
        array[i] = (float)line[i];
        _min_ = (line[i] < _min_ ? line[i] : _min_);
        _max_ = (line[i] > _max_ ? line[i] : _max_);
      }

      fwrite(array, sizeof(float), size[0], outfile);
    }

    free(array);

    fclose(outfile);

    if (min != NULL)
      *min = _min_;
    if (max != NULL)
      *max = _max_;
  }

  else
    return 1;
}
