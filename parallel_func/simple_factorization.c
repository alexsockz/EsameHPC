#include "stencil_template_parallel.h"

uint simple_factorization(uint A, int *Nfactors, uint **factors)
/*
 * rought factorization;
 * assumes that A is small, of the order of <~ 10^5 max,
 * since it represents the number of tasks
 #
 */
{
  int N = 0;
  int f = 2;
  uint _A_ = A;

  while (f < A)
  {
    while (_A_ % f == 0)
    {
      N++;
      _A_ /= f;
    }

    f++;
  }

  *Nfactors = N;
  uint *_factors_ = (uint *)malloc(N * sizeof(uint));

  N = 0;
  f = 2;
  _A_ = A;

  while (f < A)
  {
    while (_A_ % f == 0)
    {
      _factors_[N++] = f;
      _A_ /= f;
    }
    f++;
  }

  *factors = _factors_;
  return 0;
}
