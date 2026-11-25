/* NOTE: STATIC_LEX should increase execution speed considerably,
 * but may not be feasible for large values of v.
 */

#include <stdlib.h>

#ifdef ASSERTIONS
#include <assert.h>
#endif

#include "common.h"
#include "matrix.h"
#include "lexicographic.h"

#ifdef STATIC_LEX
static int ***matrix;
static int **pc;
#endif

static int nump;
static int pcsize;

int init_lexicographic(int p)
{
#ifdef STATIC_LEX
  int i, j, k;
  int counter;
#endif

#ifdef ASSERTIONS
  assert(p > 0);
#endif

  nump = p;
  pcsize = nump * (nump - 1) * (nump - 2) / 6;

#ifdef STATIC_LEX
  if (!(pc = matrix_2d(3, pcsize)))
    return FALSE;
  if (!(matrix = matrix_3d(nump, nump, nump))) {
    cleanup_lexicographic();
    return FALSE;
  }

  /* set up the illegal values */
  for (i=0; i < nump; ++i)
    for (j=0; j < nump; ++j)
      matrix[i][i][j] = matrix[i][j][i] = matrix[j][i][i] = -1;

  for (i=0, counter=0; i < nump; ++i)
    for (j=i+1; j < nump; ++j)
      for (k=j+1; k < nump; ++k, ++counter) {
	matrix[i][j][k] = matrix[i][k][j] = matrix[j][i][k] =
	  matrix[j][k][i] = matrix[k][i][j] = matrix[k][j][i] = counter;
#ifdef ASSERTIONS
	assert(counter < pcsize);
#endif
	pc[0][counter] = i;
	pc[1][counter] = j;
	pc[2][counter] = k;
      }
#endif

  return TRUE;
}

void cleanup_lexicographic(void)
{
#ifdef STATIC_LEX
  matrix_free_2d(pc, 3, pcsize);
  matrix_free_3d(matrix, nump, nump, nump);
  pc = NULL;
  matrix = NULL;
#endif
}

int p_to_lex(int p1, int p2, int p3)
{
#ifndef STATIC_LEX
  int i, tmp, count = 0;
#endif

#ifdef ASSERTIONS
  assert(p1 < nump);
  assert(p1 >= 0);
  assert(p2 < nump);
  assert(p2 >= 0);
  assert(p3 < nump);
  assert(p3 >= 0);
  assert(p1 != p2);
  assert(p1 != p3);
  assert(p2 != p3);
#endif
#ifdef STATIC_LEX
  return matrix[p1][p2][p3];
#else
  /* Sorting three values makes the numbering independent of input order. */
  if (p1 > p2) { tmp = p1; p1 = p2; p2 = tmp; }
  if (p2 > p3) { tmp = p2; p2 = p3; p3 = tmp; }
  if (p1 > p2) { tmp = p1; p1 = p2; p2 = tmp; }

  for (i=1; i <= p1; ++i)
    count += (nump - i) * (nump - i - 1) / 2;
  for (i=(p1+2); i <= p2; ++i)
    count += nump - i;
  count += (p3 - p2 - 1);
  return count;
#endif
}

void lex_to_p(int lex, int *p1, int *p2, int *p3)
{
#ifndef STATIC_LEX
  int i, tmp, bound;
#endif

#ifdef ASSERTIONS
  assert(lex < pcsize);
  assert(lex >= 0);
  assert(p1);
  assert(p2);
  assert(p3);
#endif
#ifdef STATIC_LEX
  *p1 = pc[0][lex];
  *p2 = pc[1][lex];
  *p3 = pc[2][lex];
#else
  for (i=1; i < nump; ++i) {
    tmp = (nump - i) * (nump - i - 1) / 2;
    if (lex < tmp)
      break;
    lex -= tmp;
  }
  *p1 = i-1;

  bound = nump-1;
  for (i=*p1+1; i < bound; ++i) {
    tmp = nump - i - 1;
    if (lex < tmp)
      break;
    lex -= tmp;
  }
  *p2 = i;

  *p3 = *p2 + 1 + lex;
#endif
}
