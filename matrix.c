#include <stdlib.h>
#include "common.h"
#include "matrix.h"

int **matrix_2d(int n, int m)
{
  int **matrix;
  int i, j;

  if (n <= 0 || m <= 0)
    return NULL;

  if (!(matrix = calloc(n, sizeof(int*))))
    return NULL;

  for (i=0; i < n; ++i)
    if (!(matrix[i] = calloc(m, sizeof(int)))) {
      /* could not allocate, so free all the old memory */
      for (j=0; j < i; ++j)
	free(matrix[j]);
      free(matrix);
      return NULL;
    }

  return matrix;
}

int ***matrix_3d(int n, int m, int p)
{
  int ***matrix;
  int i, j;

  if (n <= 0 || m <= 0 || p <= 0)
    return NULL;

  if (!(matrix = calloc(n, sizeof(int**))))
    return NULL;

  /* now create 2D matrices, and store them appropriately */
  for (i=0; i < n; ++i)
    if (!(matrix[i] = matrix_2d(m, p))) {
      /* delete the memory */
      for (j=0; j < i; ++j)
	matrix_free_2d(matrix[j], m, p);
      free(matrix);
      return NULL;
    }

  return matrix;
}

void matrix_free_2d(int **matrix, int n, int m)
{
  int i;

  (void)m;
  if (!matrix)
    return;
  for (i=0; i < n; ++i)
    free(matrix[i]);
  free(matrix);
}

void matrix_free_3d(int ***matrix, int n, int m, int p)
{
  int i;

  if (!matrix)
    return;
  for (i=0; i < n; ++i)
    matrix_free_2d(matrix[i], m, p);
  free(matrix);
}
