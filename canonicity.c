#ifdef ASSERTIONS
#include <assert.h>
#endif

#include <stdlib.h>

#include <string.h>

#include "common.h"
#include "backtrack.h"
#include "matrix.h"

#ifdef STATS
#include "stats.h"
#endif

#include "canonicity.h"

/* static member variables */
static int p;
static int maxb;
static int *bucketsort;
static int *rperm;
static int *cperm;
static int **cells;

/* static member function declarations */
static int next_perm_on_pos(int*, int, int, int*);
static void recompute(int, struct incidence_matrix*);
static int lex_compare(struct incidence_matrix*, int, int, int*);

int init_canonicity(int points)
{
  p = points;
  maxb = (int)(((double)p/2)*(p/3.0));

  bucketsort = 0;
  rperm = 0;
  cperm = 0;
  cells = 0;

  if (!(bucketsort = calloc(p, sizeof(int)))) {
    cleanup_canonicity();
    return FALSE;
  }

  if (!(rperm = calloc(p, sizeof(int)))) {
    cleanup_canonicity();
    return FALSE;
  }

  if (!(cperm = calloc(maxb, sizeof(int)))) {
    cleanup_canonicity();
    return FALSE;
  }

  if (!(cells = matrix_2d(p+1, maxb+1))) {
    cleanup_canonicity();
    return FALSE;
  }

  return TRUE;
}

void cleanup_canonicity(void)
{
  if (bucketsort)
    free(bucketsort);
  if (cperm)
    free(cperm);
  if (rperm)
    free(rperm);
  if (cells)
    matrix_free_2d(cells, p+1, maxb+1);
  bucketsort = NULL;
  cperm = NULL;
  rperm = NULL;
  cells = NULL;

}

/* this function, given a permutation and a position to change in
   the permutation, calculates the next permutation. TRUE is returned
   if everything went well, and FALSE is returned if there are no more
   permutations */
static int next_perm_on_pos(int *perm, int size, int pos, int *firstposchanged)
{
  int i, highest, index, flg;

#ifdef ASSERTIONS
  assert(pos < p);
#endif

  /* clear out the bucket sort */
  memset(bucketsort, 0, sizeof(int) * p);

  /* find the index of the first element to change */
  /* the first element that we can change <= pos has to be the first element
     that is less than the highest element in the remainder of the array.
     (e.g. if we have 0 4 5 1 3 2, and pos is 2 (5), we cannot change the
     permutation at pos, because the highest element in the array is less
     than pos. however, we can change the permutation at pos 1 (4), because
     4 < 5, and 5 is the highest element in the subarray 2..5 */
  bucketsort[perm[pos]] = 1;

  /* we initially scan through the array, sorting the elements, and
     finding the highest */
  highest = pos;
  for (i=pos+1; i < size; ++i) {
    /* we can sort at the same time */
#ifdef ASSERTIONS
    assert(perm[i] < p);
#endif
    bucketsort[perm[i]] = 1;

    /* we must also swap as required to maintain the highest element */
    if (perm[i] > perm[highest])
      highest = i;
  }

  /* now we determine the index at which we can first create a next
     permutation */
  for (index=pos; index >= 0 && (perm[highest] <= perm[index]); --index) {
    /* we can sort at the same time */
#ifdef ASSERTIONS
    assert(perm[index] < p);
#endif
    bucketsort[perm[index]] = 1;

    /* we must also swap to keep track of the highest element
       in i..size */
    if (perm[index] > perm[highest])
      highest = index;
  }

  /* if the index is less than 0, then we could not find a new perm */
  if (index < 0)
    return FALSE;

  /* must record the last index */
  bucketsort[perm[index]] = 1;

  /* now all the elements are sorted, and we know where to begin adding */
  for (i=0, pos=index+1, flg=0; !flg || (i < p && pos < size); ++i) {
    /* we want to find the element higher than perm[index] to put in index */
    if (!flg && bucketsort[i] && i > perm[index]) {
      flg = 1;
      perm[index] = i;
      continue;
    }

    if (bucketsort[i]) {
      perm[pos] = i;
      ++pos;
    }
  }

  /* we have now constructed the next permutation */
  if (firstposchanged)
    *firstposchanged = index;
  return TRUE;
}

static void recompute(int row, struct incidence_matrix *I)
{
  int i, j, k, bound, tmp, cellpos;

#ifdef ASSERTIONS
  assert(row < p);
  assert(row >= 0);
#endif

  /* we will create the cells at the same time, if required */
  cellpos = 0;

  for (i=0; !i || cells[row][i]; ++i) {
    /* calculate the index of the first column in the cell */
    bound = k = (i > 0 ? cells[row][i-1]+1 : 0);

    /* now we want to swap all columns with 1s to the left of the cell,
       and all with 0s to the right */
    /* iterate over the columns in the cell */
    for (j=bound+1; j <= cells[row][i]; ++j) {
      if (gbit(I->cols[cperm[j]].v, rperm[row]) == 0) {
	if (gbit(I->cols[cperm[k]].v, rperm[row]) == 1)
	  k = j;
      }
      else {
	/* we swap cols k and j */
	tmp = cperm[k];
	cperm[k] = cperm[j];
	cperm[j] = tmp;

	/* increment k */
	++k;
      }
    }

    /* now we have k, so we can partition the cell if required */
    if ((row+1) < p) {
      /* at this point, k is an index to the column that has the first zero
	 entry in this cell (or one past the last column in the cell if the
	 cell is completely filled with ones). we can use this information
	 to partition the cell */

      if (k == bound) {
	/* there are no ones in this cell. just copy the cell across */
#ifdef ASSERTIONS
	assert(cellpos <= maxb);
#endif
	cells[row+1][cellpos] = cells[row][i];
	++cellpos;
      }
      else if (k > bound) {
	/* k has been moved ahead, so there are ones in the cell.
           the cell must either be partitioned or copied depending on
	   whether or not there are any 0s. */
#ifdef ASSERTIONS
	assert(cellpos <= maxb);
#endif
	cells[row+1][cellpos] = k-1;
	++cellpos;

	/* if k is not equal to cells[row][i]+1, then there are some zeros
	   in the cell, and thus the cell must be partitioned */
	if (k <= cells[row][i]) {
#ifdef ASSERTIONS
	  assert(cellpos <= maxb);
#endif
	  cells[row+1][cellpos] = cells[row][i];
	  ++cellpos;
	}
      }
    } 
  }

  /* if we are adding cells, we must put the termination in place */
  if ((row+1) < p) {
#ifdef ASSERTIONS
    assert(cellpos <= maxb);
#endif
    cells[row+1][cellpos] = 0;
  }
}

static int lex_compare(struct incidence_matrix *I, int rrow, int prow,
		       int *onesperrow)
{
  int i, b1, b2;

  /* we can immediately tell which row is lex smaller if the # of ones
     is not the same */
  if (onesperrow[rrow] > onesperrow[prow])
    return -1;
  else if (onesperrow[rrow] < onesperrow[prow])
    return 1;

  for (i=0; i < I->maxb; ++i) {
    b1 = gbit(I->cols[i].v, rrow);
    b2 = gbit(I->cols[cperm[i]].v, prow);

    if (b1 > b2)
      return -1;
    else if (b2 > b1)
      return 1;
  }

  return 0;
}

int is_canonical(int row, struct incidence_matrix *I, int **oldcells,
		 int *onesperrow, long *automsize)
{
  int i, j, firstposchanged;
  int continueflg, doneflg;
  int result, pos;

  /* the automctr starts at 1 because we never check the identity perm,
     which is always an automorphism */
  long automctr = 1L;
  int automflg;

  /* we initialize the automsize to 0 */
  if (automsize)
    *automsize = 0L;

#ifdef STATS
  /* tell the stats module that we're performing a canonicity check */
  stats_record_canonicity_check(row-1);
#endif

  /* set up the permutations */
  for (i=0; i < p; ++i)
    rperm[i] = i;
  for (i=0; i < maxb; ++i)
    cperm[i] = i;

  /* copy the cell partition matrix */
  for (i=0; i <= p; ++i)
    for (j=0; j <= maxb; ++j)
      cells[i][j] = oldcells[i][j];
  /*  memcpy(cells, oldcells, sizeof(int) * (p+1) * (maxb + 1)); */

  /* continueflg indicates that we don't want to get the next permutation
     at this point - we just want to continue with the current rperm.
     doneflg indicates whether or not we are done. */
  continueflg = doneflg = 0;
  pos = p-1;

  while (continueflg ||
	 (!doneflg && next_perm_on_pos(rperm, row, pos, &firstposchanged))) {
    /* indicate that we have an automorphism, unless we have good
       reason to believe the contrary */
    automflg = 1;

    for (i=firstposchanged; i < row; ++i) {
#ifdef STATS
      /* if we are recording stats, inform the program that we are performing
	 an analysis of a partial design (a column recomputation, cell
	 repartitioning, etc...) */
      stats_record_canon_pd();
#endif

      /* we must recompute the column permutation */
      recompute(i, I);

      /* now we need to compare the original row in the incidence matrix
	 to the permuted new row */
      result = lex_compare(I, i, rperm[i], onesperrow);

      if (result < 0) {
	/* I is less than I', and thus we must backtrack on the permutation,
	   as any extensions of the permutation would be less than I' */
	pos = i;
	automflg = 0;
	break;
      }
      else if (result > 0) {
	/* I' is less than I, and then clearly I cannot be canonical */
#ifdef STATS
	/* tell the stats module that we have rejected a design */
	stats_record_isomorphism_rejection(row-1);
#endif
	return FALSE;
      }

      /* set pos to try the next permutation */
      pos = p-1;
    }

    automctr += automflg;
  }

  /* if we reach this point, we have tried all relevant permutations,
     and the design is thus far canonical */
  if (automsize)
    *automsize = automctr;
  return TRUE;
}
