#include <stdlib.h>

#ifdef ASSERTIONS
#include <assert.h>
#endif

#include <string.h>

#include "common.h"
#include "lexicographic.h"
#include "canonicity.h"
#include "matrix.h"

#ifdef STATS
#include "stats.h"
#endif

#include "backtrack.h"

/* static structures */

/* incidence matrix */
static struct incidence_matrix I;

/* matrix of "cells" as described by Paul Denny's master thesis - using
   a matrix as opposed to a vector simplifies backtracking on points */
static int **cells;

/* matrix of "cell contents", to speed up computation */
static int **cellcs;

/* number of 1s per row, for backtracking */
static int *onesperrow;

/* NOTE: in both the triples and pairs arrays, when a triple or a pair is added to
   a design by adding point p, then p+1 is stored at the index of the triple and
   at the index of the pair. This is because if we did not add the 1, anything
   created by adding point 0 (this is theoretically impossible, but still should
   be taken into consideration) would result in a 0 in the array, which is the
   same as saying that the point / triple is not in our design */

/* array indexed by lex number of triples in order to store status information
   about them */
static int *triples;
static int triplesize;

/* number of points */
static int p;

/* max number of 1s per row */
static int maxr;

/* max number of blocks */
static int maxb;

/* upper and lower bounds on r and b */
static struct bounds *bnds;

/* keeps track of the total number of 1s in the matrix */
static int numones;

/* static method declarations */
static int remove_point(int);
static int add_point(int);
static int calculate_cells(int);
static int lex_compare(int, int);
static int is_complete(void);
static int row_backtrack(int);

/* temporary pairs array to use during backtracking on a row
   to determine which pairs are in the matrix for this row */
static int *tpairs;

/* NOTE: the reason that the following methods only involve points not in
   the columns is because they are used during row backtracking, and hence
   the points have not yet been recorded in the point vectors */

/* this method, given a column, for a point not in the column, checks to see
   if adding the point to the column will violate the pair
   constraint of PSTS - returns TRUE if a duplicate pair would be formed */
static int pair_duplicate(int);

/* this method, given a column, for a point not in the column, records all
   the pairs that would be formed by placing this point in the column */
static void pair_record(int);

/* this method, given a column, for a point not in the column, removes all
   the pairs that would be formed by placing this point in the column */
static void pair_remove(int);

/* this method, given a column and a point, determines if adding the point
   to the column will form a Pasch */
static int forms_pasch(int, int);

/* this method converts a block to a lex numbering - an upper bound for
   the number of points to be considered can be passed in as the int
   parameter */
static int block_to_lex(point_bits, int);

int set_default_bounds(struct bounds *b, int points)
{
  /* special cases */
  switch (points) {
  case 6:
  case 7:
  case 12:
  case 13:
    b->lor = 1;
    b->upr = (points-1)/2;
    b->lob = 1;
    b->upb = (int)(((double)((points-1)/2))*(points/3.0));
    return TRUE;
  };

  /* now we handle regular cases */
  switch (points % 6) {
  case 1:
  case 3:
  case 6:
    b->lob = b->upb = (points * points - points) / 6;
    break;
  case 0:
  case 2:
    b->lob = b->upb = (points * points - 2 * points) / 6;
    break;
  case 4:
    b->lob = ((points - 1) * (points - 2)) / 6;
    b->upb = (points * points - 2 * points - 2) / 6;
    break;
  case 5:
    b->lob = ((points - 2) * (points - 3)) / 6;
    b->upb = (points * points - points - 8) / 6;
  };

  /* set r */
  switch (points % 6) {
  case 1:
  case 3:
    b->lor = b->upr = (points - 1) / 2;
    break;
  case 0:
  case 2:
    b->lor = b->upr = (points - 2) / 2;
    break;
  case 4:
  case 5:
    b->lor = 1;
    b->upr = (points - 1) / 2;
  };

  return TRUE;
}
  

int init_backtrack(int ps, struct bounds *b)
{
  int i, j;

  /* initialize int values */
  p = ps;
  I.maxb = maxb = (int)(((double)p/2)*(p/3.0));
  I.b = 0;
  maxr = (p-1)/2;
  triplesize = (p)*(p-1)*(p-2)/6;
  numones = 0;

  /* set the bounds */
  bnds = b;

  /* initialize for emergency cleanup */
  I.cols = 0;
  cells = 0;
  cellcs = 0;
  onesperrow = 0;
  triples = 0;
  tpairs = 0;

  /* initialize the canonicity library */
  if (!init_canonicity(p)) {
    cleanup_backtrack();
    return FALSE;
  }

  /* now try to create everything */
  if (!(I.cols = calloc(maxb, sizeof(struct incidence_col)))) {
    cleanup_backtrack();
    return FALSE;
  }

  if (!(cells = matrix_2d(p+1, maxb+1))) {
    cleanup_backtrack();
    return FALSE;
  }

  if (!(cellcs = matrix_2d(p, maxb))) {
    cleanup_backtrack();
    return FALSE;
  }

  if (!(onesperrow = calloc(p, sizeof(int)))) {
    cleanup_backtrack();
    return FALSE;
  }

  if (!(triples = calloc(triplesize, sizeof(int)))) {
    cleanup_backtrack();
    return FALSE;
  }

  if (!(tpairs = calloc(p, sizeof(int)))) {
    cleanup_backtrack();
    return FALSE;
  }

  /* set up the incidence matrix */
  for (i=0; i < maxb; ++i) {
    I.cols[i].v = 0;
    I.cols[i].c = 0;
    for (j=0; j < 3; ++j)
      I.cols[i].n[j] = -1;
  }

  /* set up the initial cell */
  calculate_cells(0);

  /* set up the cell contents to be initially empty */
  for (i=0; i < p; ++i)
    for (j=0; j < maxb; ++j)
      cellcs[i][j] = 0;

  for (i=0; i < maxr; ++i)
    onesperrow[i] = 0;

  /* set up the triples status */
  for (i=0; i < triplesize; ++i)
    triples[i] = 0;

  /* all initialized */
  return TRUE;
}

void cleanup_backtrack(void)
{
  /* free the canonicity lib */
  cleanup_canonicity();

  /* dealloc all the memory */
  if (I.cols)
    free(I.cols);
  if (cells)
    matrix_free_2d(cells, p+1, maxb+1);
  if (cellcs)
    matrix_free_2d(cellcs, p, maxb);
  if (onesperrow)
    free(onesperrow);
  if (triples)
    free(triples);
  if (tpairs)
    free(tpairs);
  I.cols = NULL;
  cells = NULL;
  cellcs = NULL;
  onesperrow = NULL;
  triples = NULL;
  tpairs = NULL;

}

/* the main backtracking function - begins backtracking by adding point 0
   and then subsequently adding and removing points */
int backtrack(bool (*complete)(struct incidence_matrix*, long, void*),
#ifdef DEBUG
	      void (*partial)(struct incidence_matrix*, void*),
#endif
	      int *cancheck,
	      void *userdata)
{
  int point = 0, oldpoint = -1;
  int dir;        /* TRUE - forward, FALSE - backward */
  int flg, flg2;
  long automsize = 1;

  while (point >= 0) {
    /* determine if we need to calculate the cell partition - save time
       by avoiding doing so if possible */
    if (point != oldpoint)
      calculate_cells(point);
    oldpoint = point;

    /* REMOVE ANY POINT DATA for this point */
    remove_point(point);

    /* create a new row configuration and determine if we were successful */
    dir = row_backtrack(point);

    /* at this point, we can imply the special bound on r for the first row */
    if (onesperrow[0] < (3 * bnds->lob / p))
      break;

    /* we can also immediately tell if this design will be able to be completed;
       we have a lower bound for b of bnds->lob. This means that we must have at least
       3*bnds->lob 1s in the matrix. since every row r+1 in I has the same or less 1s
       than r, we can find an upper bound for the number of ones */
    if ((numones + (onesperrow[point] * (p - point))) < 3 * bnds->lob)
      dir = 0;

    /* if we succeed, we will tell the backtracking to add the next point */
    if (dir) {
      /* POINT HAS BEEN ADDED - update the data structs appropriately */
      /* we now add the row information to the incidence matrix */
      add_point(point);

      /* we make sure that the row added was lexicographically smaller
	 than the previous row - if it wasn't, we ignore this design.
	 also, the design must be canonical! */
      /* report to the calling code that a new design has been created,
	 if appropriate */
      flg = (!point || (lex_compare(point, point-1) <= 0));

      /* if this is a design with all points added, it must be complete */
      /* we have not yet incremented point, so we must compare to p-1 */
      if (flg && point == (p-1))
        flg &= is_complete();

      /* the design must be canonical */
      /* nothing ever gets rejected on the first three points */
      if (flg && cancheck[point]) {
	flg2 = is_canonical(point+1, &I, cells, onesperrow, &automsize);
	flg &= flg2;
      }

      /* now if the rows are in lex order, and the design is canonical,
	 we can process further */
      if (flg) {
	++point;

	/* we process according to whether this is a complete
	   or partial design */
	if (point == p) {
	  /* we have already checked that this design is complete before checking
	     canonicity */
	  /* we will update the lower bound on b appropriately, and only
	     report designs that satisfy the conditions */
	  if (bnds->lob < I.b)
	    bnds->lob = I.b;

	  if (I.b >= bnds->lob) {
#ifdef STATS
	    stats_record_complete();
#endif
	    if (complete && !complete(&I, automsize, userdata))
	      return FALSE;
	  }
	}
	else {
	  /* this design does not contain all the points; hence it is
	     considered a partial design */
#ifdef STATS
	  stats_record_partial();
#endif

#ifdef DEBUG
	  if (partial)
	    partial(&I, userdata);
#endif
	}

	/* as we move forward, we must reset the cell contents */
	/* this is required so that row_backtrack doesn't try to backtrack
	   on old data stored in a row */
	if (point < p) {
	  memset(cellcs[point], 0, sizeof(int) * maxb);
	  onesperrow[point] = 0;
	}
      }
    }
    /* if we failed, we will immediately resume backtracking */
    else {
      --point;
    }
    
    /* if we have tried to advance past the end, we automatically backtrack */
    if (point == p)
      --point;
  }
  
  return TRUE;
}

/* Given a point number, remove all data from the incidence matrix
   regarding the point */
static int remove_point(int point)
{
  int i, j, bound;
  int tmp, index;

  /* this may not be the most efficient way to do it, but it will
     work for now */
  for (i=0; !i || cells[point][i]; ++i) {
#ifdef ASSERTIONS
    assert(i <= maxb);
#endif

    /* get the index into the incidence matrix of the first column in the cell */
    index = (i > 0 ? cells[point][i-1]+1 : 0);

    /* check if this cell contains anything */
    if (cellcs[point][i] || gbit(I.cols[index].v, point)) {
      /* if this column contains a complete triple, we must remove the triple */
      if (I.cols[index].c == 3) {
	tmp = p_to_lex(I.cols[index].n[0], I.cols[index].n[1], I.cols[index].n[2]);
#ifdef ASSERTIONS
	assert(tmp >= 0);
	assert(tmp < triplesize);
#endif
	triples[tmp] = 0;
	--(I.b);
#ifdef ASSERTIONS
	assert(I.b >= 0);
#endif
      }

      /* remove all data from the incidence matrix */
      bound = index + cellcs[point][i];
      for (j=index; j < bound; ++j) {
#ifdef ASSERTIONS
	assert(j < maxb);
#endif
	sbit(I.cols[j].v, point, 0);
	I.cols[j].n[I.cols[j].c - 1] = -1;
	--(I.cols[j].c);

	/* record a 1 being removed from I */
	--numones;
      }
    }
  }
  
  return TRUE;
}

/* Given a point number, transfer the information from the cellcs
   array to the incidence matrix */
static int add_point(int point)
{
  int i, j, bound, lowerbound, index;
  int pp1, tmp;

  for (i=0; !i || cells[point][i]; ++i) {
#ifdef ASSERTIONS
    assert(i <= maxb);
#endif
    bound = cellcs[point][i];
    lowerbound = (i > 0 ? cells[point][i-1]+1 : 0);
    for (j=0; j < bound; ++j) {
      index = lowerbound + j;
#ifdef ASSERTIONS
      assert(index < maxb);
#endif
      /* set the bit in the bitmask */
      sbit(I.cols[index].v, point, 1);

      /* record a 1 being added to I */
      ++numones;

      /* add the point to the array */
      I.cols[index].n[I.cols[index].c] = point;
      ++(I.cols[index].c);
#ifdef ASSERTIONS
      assert(I.cols[index].c <= 3);
#endif

      pp1 = point + 1;
      /* if this adds a triple, we must record the triple */
      if (I.cols[index].c == 3) {
	tmp = p_to_lex(I.cols[index].n[0], I.cols[index].n[1], point);
#ifdef ASSERTIONS
	assert(tmp >= 0);
	assert(tmp < triplesize);
#endif
	triples[tmp] = pp1;
	++(I.b);
#ifdef ASSERTIONS
	assert(I.b <= I.maxb);
#endif
      }
    }
  }

  return TRUE;
}

int calculate_cells(int row)
{
  int i, j;

#ifdef ASSERTIONS
  assert(row >= 0);
  assert(row < p+1);
#endif

  /* special treatment for row 1 */
  if (row == 0) {
    /*    cells[0][0] = maxb-1; */
    cells[0][0] = bnds->upb - 1;
    cells[0][1] = 0;
    return TRUE;
  }

  /* now calculate using the i-1st row */
  for (i=0, j=0; !i || cells[row-1][i]; ++i) {
#ifdef ASSERTIONS
    assert(j <= maxb);
#endif
    /* was cellcs[row-1][j] - this seems incorrect? i makes sense */
    cells[row][j] = (i > 0 ? cells[row-1][i-1]+1 : 0) + cellcs[row-1][i] - 1;

    /* we do not add duplicates, which may occur as a result of this algorithm,
       and we do not add negative numbers, which may result in the first
       calculation */
    if ((!j || cells[row][j] > cells[row][j-1]) && cells[row][j] >= 0) {
#ifdef ASSERTIONS
      if (j > 0)
	assert(cells[row][j] > cells[row][j-1]);
#endif
      ++j;
    }

    if (!j || cells[row][j-1] != cells[row-1][i]) {
#ifdef ASSERTIONS
      assert(j <= maxb);
#endif
      cells[row][j] = cells[row-1][i];
#ifdef ASSERTIONS
      if (j > 0)
	assert(cells[row][j] > cells[row][j-1]);
#endif
      ++j;
    }
  }

  /* put the termination */
#ifdef ASSERTIONS
      assert(j <= maxb);
#endif
  cells[row][j] = 0;

  /* we have successfully partitioned */
  return TRUE;
}

/* compare two rows and determine which one is lexicographically smaller
   in our implementation, rows with less 1s are lexicographically smaller
   than rows with more 1s. hence 001000 is less than 111111.
*/
static int lex_compare(int row1, int row2)
{
  int i;
  int b1, b2;

#ifdef ASSERTIONS
  assert(row1 >= 0);
  assert(row2 >= 0);
  assert(row1 < p);
  assert(row2 < p);
#endif

  /* if one has more ones than the other, then we immediately know lex order */
  if (onesperrow[row1] != onesperrow[row2])
    return (onesperrow[row1] > onesperrow[row2] ? 1 : -1);

  /* determine which one has a 1 earlier in the row - this one will be
     lexicographically smaller */
  /* this could probably be optimized to not iterate all the way to maxb */
  for (i=0; i < maxb; ++i) {
    b1 = gbit(I.cols[i].v, row1);
    b2 = gbit(I.cols[i].v, row2);
    if (b1 > b2)
      return 1;
    if (b2 > b1)
      return -1;
  }

  /* they are the same */
  return 0;
}

/* is_complete checks to see if a design is complete - i.e. that it has
   3 1s in every block */
static int is_complete(void)
{
  int i;

  for (i=0; i < maxb; ++i) {
    if (I.cols[i].c && I.cols[i].c < 3)
      return FALSE;
    if (!(I.cols[i].c))
      return TRUE;
  }

  return TRUE;
}
  

int row_backtrack(int row)
{
  int bound;
  int index;
  int lastcell = -1;
  int onestoadd;
  int celltoadd = -1;
  int i;
  int totones;
  int *newrow = cellcs[row];

  /* used in calculating initial configurations */
  int firstcol, secondcol;
  int firstcell, tmpcell;

#ifdef ASSERTIONS
  assert(row >= 0);
  assert(row < p);
#endif

  /* calculate the index of the last cell - it may be better to precalculate
     this later */
  bound = maxb+1;
  for (i=1; i < bound; ++i)
    if (cells[row][i] == 0) {
      lastcell = i-1;
      break;
    }

  /* remove all pairs */
  memset(tpairs, 0, p * sizeof(int));

  /* *** We have 4 cases to consider before we start adding ones *** */
  /* if there are no 1s, we add the upper bound  */
  if (onesperrow[row] == 0) {
    onestoadd = (row ? onesperrow[row-1] : bnds->upr);
    celltoadd = 0;

    if (row) {
      /* We are adding the same number of ones as the previous row, but the
	 first one can come no earlier in the row configuration than the first one
	 in the previous row. If the first one in this row comes in the same column
	 as the first one in the previous row (we try this first, so to cover all
	 the possible lex. configs), the second one in this row must come AFTER the second
	 one in the previous row. If it came earlier, this row will be lex. invalid. If it
	 came at the same position, the pair constraint would be violated (as the first one
	 comes in the same column as the one in the previous row). */

      /* get the index of the column containing the first one in the previous row */
      firstcol = -1;
      tmpcell = -1;
      for (i=0; !i || cells[row-1][i]; ++i)
	if (cellcs[row-1][i]) {
	  firstcol = (i ? cells[row-1][i-1]+1 : 0);
	  tmpcell = i;
	  break;
	}
#ifdef ASSERTIONS
      assert(firstcol >= 0);
      assert(firstcol < I.maxb);
      assert(tmpcell >= 0);
#endif

      /* get the cell in this row that contains the column */
      firstcell = -1;
      for (i=0; !i || cells[row][i]; ++i)
	if (firstcol <= cells[row][i]) {
	  firstcell = i;
	  break;
	}
#ifdef ASSERTIONS
      assert(firstcell >= 0);
#endif

      /* Now if this column contains a complete triple, the above strategy is useless. We simply
	 start adding to the next cell. */
      if (I.cols[firstcol].c == 3)
	celltoadd = firstcell + 1;

      /* If we are only trying to add one 1, then we simply proceed with this cell */
      else if (onestoadd == 1)
	celltoadd = firstcell;

      /* Otherwise we are adding multiple ones, and can add one directly to the first column */
      else {
	/* Add the one and record the pairs */
	cellcs[row][firstcell] = 1;
	pair_record(firstcol);
	--onestoadd;
	++onesperrow[row];

	/* Now we find the next column in the previous row that contains a 1 */
	/* If the cell we previously calculated contains more than one 1, this is trivial */
	if (cellcs[row-1][tmpcell] > 1)
	  celltoadd = tmpcell + 1;

	else {
	  secondcol = -1;
	  for (i=tmpcell+1; !i || cells[row-1][i]; ++i)
	    if (cellcs[row-1][i]) {
	      secondcol = (i ? cells[row-1][i-1]+1 : 0);
	      break;
	    }
#ifdef ASSERTIONS
	  assert(secondcol >= 0);
	  assert(secondcol < I.maxb);
#endif

	  /* Find the cell in this row that contains the column we calculated */
	  celltoadd = -1;
	  for (i=0; !i || cells[row][i]; ++i)
	    if (secondcol <= cells[row][i]) {
	      celltoadd = i;
	      break;
	    }
#ifdef ASSERTIONS
	  assert(celltoadd >= 0);
#endif
	}
      }
    }
  }

  /* if all the ones are in the last cell, then we remove all ones and
     indicate that we start over with one less */
  else if (newrow[lastcell] == onesperrow[row]) {
    onestoadd = onesperrow[row] - 1;
    onesperrow[row] = 0;
    newrow[lastcell] = 0;
    celltoadd = 0;
  }

  /* there are some ones in the last cell, which means that we need to
     remove them, and move the first 1 not in the last cell forward */
  /* OR... there are no ones in the last cell, in which case, we move
     the last 1 forward by a cell */
  else {
    onestoadd = newrow[lastcell] + 1;
    newrow[lastcell] = 0;

    /* remove the previous one */
    for (i=lastcell-1; i >= 0; --i) {
      if (newrow[i] > 0) {
	--newrow[i];
	index = (i ? (cells[row][i-1]+1) : 0);
	celltoadd = i+1;
	break;
      }
    }

    /* now we must record pairs for the 1s already in place */
    bound = celltoadd - 1;
    for (i=0; i < bound; ++i)
      if (cellcs[row][i]) {
	/* record the ones */
	index = (i > 0 ? cells[row][i-1]+1 : 0);
	pair_record(index);
      }
    
    onesperrow[row] -= onestoadd;
#ifdef ASSERTIONS
    assert(celltoadd > 0);
#endif
  }

  /* if we cannot accomodate the number of 1s that we are trying to add,
     we will try with one less, and thus we must loop here */
  while (1) {
    /* we have to make sure that the number of 1s that we want to add is
       permitted */
    totones = onestoadd + onesperrow[row];
    if (totones > bnds->upr || totones < bnds->lor)
      return FALSE;

    /* we add as many ones as possible to the cells in order */
    for (i=celltoadd; i <= lastcell; ++i) {
      /* what we are doing here is finding a bound - this bound indicates
	 the number of 1s that we can add to a cell - for instance, if our
	 cells are partitioned as follows: 0 2 6 7
	 then we can fit 6-2 = 4 elements in cell 3
	 and we can fit 0 - - 1 = 1 elements in cell 1
      */
      bound = (cells[row][i] - (i > 0 ? cells[row][i-1] : -1));

      /* index in the incidence matrix of the first col in this cell */
      index = (i > 0 ? cells[row][i-1]+1 : 0);
      
      /* we cannot add to complete cells */
      if (I.cols[index].c == 3)
	continue;

      /* if there are already any 1s in the cell, then we can only add one 1
	 to the cell - to add  more would add the same pair twice */
      if (I.cols[index].c > 0 && onestoadd > 0) {
	/* make sure that adding a 1 here will not violate the pairs constraint
	   of a PSTS */
	if (pair_duplicate(index))
	  continue;

	/* make sure that adding a 1 here will not form a Pasch! */
	if (forms_pasch(index, row))
	  continue;

	newrow[i] = 1;
	++(onesperrow[row]);
	--onestoadd;
	
	/* record the pairs */
	pair_record(index);
	
	continue;
      }
      
      /* there are no 1s in the cell - we can add as many 1s as we want to this
	 cell. if there are less 1s than the bound, we can add all 1s to this cell */
      if (onestoadd <= bound) {
	newrow[i] = onestoadd;
	onesperrow[row] += onestoadd;
	onestoadd = 0;
	break;
      }

      /* we cannot add all 1s to this cell, so we add as many ones as possible
	 to this cell and try to add the rest in future loop iterations */
      newrow[i] = bound;
      onesperrow[row] += newrow[i];
      onestoadd -= newrow[i];
    }

    /* previously, if we couldn't add all ones, we would just backtrack immediately
       on points. however, just because we cannot add all 1s as early as possible in
       the row does not mean that by shuffling 1s to the right that we cannot add all
       1s to the row, so we must try this (this was causing anti-Pasch PSTS(15) to not
       appear!) */
    /* if there are still ones to add, we try removing all the terminating ones, then
       moving the previous one forward a cell, and then readding. we cannot move the
       first one forward a cell, as that would result in an incomplete triple */
    if (onestoadd > 0 && (onesperrow[row] - newrow[lastcell] >= 1)) {
      onestoadd = onestoadd + newrow[lastcell] + 1;
      onesperrow[row] -= newrow[lastcell];

      /* remove all pairs for the last cell if there are things in the last cell */
      if (newrow[lastcell] > 0 && lastcell > 0)
	pair_remove(cells[row][lastcell-1]+1);
      newrow[lastcell] = 0;

      /* remove the previous one */
      for (i=lastcell-1; i >= 0; --i) {
	if (newrow[i] > 0) {
	  index = (i ? cells[row][i-1] + 1 : 0);
	  --newrow[i];
	  --onesperrow[row];
	  pair_remove(index);
	  celltoadd = i+1;
	  break;
	}
      }
      continue;
    }

    /* if there are still ones to add, then we couldn't do this - the cells
       couldn't accomodate the 1s */
    if (onestoadd > 0) {
      /* it is important that we remove all recorded pairs! */
      for (i=0; i <= lastcell; ++i) {
	index = (i > 0 ? cells[row][i-1]+1 : 0);
	pair_remove(index);
      }

      /* now what we want to do is clear the row and try to add 1 less */
      onestoadd = onestoadd + onesperrow[row] - 1;

#ifdef ASSERTIONS
      /* if 0, we will return with FALSE at the beginning of the next while
	 loop iteration */
      assert(onestoadd >= 0);
#endif

      /* reset the row */
      onesperrow[row] = 0;
      memset(cellcs[row], 0, sizeof(int) * maxb);
      celltoadd = 0;
      continue;
    }
    
    /* we successfully added all the ones, and have a complete row! */
    return TRUE;
  }
}

static int pair_duplicate(int col)
{
  int i;

  for (i=0; i < I.cols[col].c; ++i)
    if (tpairs[I.cols[col].n[i]])
      return TRUE;

  return FALSE;
}

static void pair_record(int col)
{
  int i;

  for (i=0; i < I.cols[col].c; ++i)
    tpairs[I.cols[col].n[i]] = 1;
}

static void pair_remove(int col)
{
  int i;

  for (i=0; i < I.cols[col].c; ++i)
    tpairs[I.cols[col].n[i]] = 0;
}

static int forms_pasch(int col, int point)
{
  point_bits block1, block2, block3, block4;
  int l1, l2;
  int i, j, k, l;
  int index1, index2, index3;

#ifdef ASSERTIONS
  assert(col >= 0);
  assert(col < maxb);
  assert(point >= 0);
  assert(point < p);
#endif

  /* make sure that a Pasch is possible */
  /* we were using I.b >= 4 as a condition for a Pasch, but note
     that when adding a point, the blocks completed by the point
     have not at this point been accounted for by I.b */
  /* we check for 2 here instead of 3, because point has not yet been
     officially added to the column */
  if (I.cols[col].c < 2)
    return FALSE;

  /* create a temporary bitset representing the modified column */
  block1 = I.cols[col].v;
  sbit(block1, point, 1);

  /* now determine out of the points making up the column which
     one has the lowest rs - since lex ordering determines that
     points further down in the matrix have lower r than points
     higher up, then this is easy */
  l1 = point;
  l2 = I.cols[col].n[1];

  /* now we iterate over the columns containing point l1 (not this column),
     and XOR them with this column and all columns containing point l2 */
  /* we can save time by iterating over cells - and since col would be the
     newest completed block, we never need to iterate further than that */
  for (i=0; (!i || cells[l1][i]) && (i < col); ++i) {
    if (cellcs[l1][i] > 0) {
      /* try all columns in this cell */
      for (j=0; j < cellcs[l1][i]; ++j) {
	/* only try complete blocks, and blocks containing this point */
	index1 = (i ? (cells[l1][i-1]+1) : 0) + j;
#ifdef ASSERTIONS
	assert(index1 >= 0);
	assert(index1 < maxb);
#endif

	/* We must check for 2, because this is the most recently added point,
	   and hence c has not yet been updated (add_point has not yet been called!) */
	if (I.cols[index1].c != 2)
	  continue;

	/* set up block2 */
	block2 = I.cols[index1].v;
	sbit(block2, point, 1);

	/* this is a complete block containing the point; now we need a third
	   block containing l2 */
	for (k=0; !k || cells[l2][k]; ++k) {
	  if (cellcs[l2][k] > 0) {
	    /* try all columns in this cell */
	    for (l=0; l < cellcs[l2][k]; ++l) {
	      /* only try complete blocks, and blocks containing this point */
	      /* and make sure that we are not using the same column twice! */
	      index2 = (k ? (cells[l2][k-1]+1) : 0) + l;
#ifdef ASSERTIONS
	      assert(index2 >= 0);
	      assert(index2 < maxb);
#endif
	      if (index2 == col || index1 == index2)
		continue;
	      if (gbit(I.cols[index2].v, l2) == 0)
		break;
	      if (I.cols[index2].c != 3)
		continue;
	      block3 = I.cols[index2].v;

	      /* at this point we have the three blocks we need to check for a
		 Pasch - the condition we have now is that all the blocks must
		 have exactly one element in common. they cannot have more
		 than one element in common, because then the pair constraint
		 of PSTS would be violated */
	      if (((block1 & block2) == 0) || ((block1 & block3) == 0)
		  || ((block2 & block3) == 0))
		continue;

	      /* check to make sure as well that the blocks don't all have
		 the same points in common */
	      if ((block1 & block2) == (block1 & block3))
		continue;

	      /* determine the 4th block in a Pasch */
	      block4 = (block1 ^ block2 ^ block3);

	      /* now we can check if this block is present - if it is, then
		 we have a Pasch */
	      /* PROBLEM: how can we determine from a bit string whether or not
		 a block is in the design??? Is there an efficient way to convert
		 between bitset and triple? */
	      if ((index3 = block_to_lex(block4, point)) < 0)
		continue;

	      /* make sure that the index is not the same as any other index */
	      if (index3 == p_to_lex(I.cols[col].n[0], I.cols[col].n[1], point)
		  || index3 == p_to_lex(I.cols[index1].n[0], I.cols[index1].n[1], point)
		  || index3 == p_to_lex(I.cols[index2].n[0], I.cols[index2].n[1],
					I.cols[index2].n[2]))
		continue;

#ifdef ASSERTIONS
	      assert(index3 >= 0);
	      assert(index3 < triplesize);
#endif

	      /* check to see if the required triple is in the design - if it is,
		 then we have a Pasch */
	      if (triples[index3] > 0)
		return TRUE;
	    }
	  }
	}
      }
    }
  }

  /* we could not find a Pasch */
  return FALSE;
}

static int block_to_lex(point_bits block, int max)
{
  int ps[3];
  int i, j, lex;

#ifdef ASSERTIONS
  assert(max < POINT_BITS);
#endif

  /* this is inefficient, but may be better than searching across maxb */
  /* we need to think about this */
  for (i=0, j=0; i <= max && j < 3; ++i)
    if (gbit(block, i) == 1) {
      ps[j] = i;
      ++j;
    }

  if (j != 3)
    return -1;

  /* convert to a lex number */
  lex = p_to_lex(ps[0], ps[1], ps[2]);
#ifdef ASSERTIONS
  assert(lex >= 0);
  assert(lex < triplesize);
#endif

  return lex;
}
