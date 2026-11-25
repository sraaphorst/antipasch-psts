/* backtrack.h
 *
 * This algorithm encompasses the backtracking algorithms of the incidence
 * matrix, and of the rows within the incidence matrix.
 */

#ifndef BACKTRACK_H
#define BACKTRACK_H

#include <stdbool.h>
#include "common.h"

/* structure for assigning bounds on r and b */
struct bounds {
  int lor, upr;
  int lob, upb;

};

/* structure of a column in the incidence matrix */
struct incidence_col {
  point_bits v;
  int n[3];  /* quick way to access triples - it is helpful to have
	       both representations - a bit string and a triple */
  int c;
};

/* structure of an incidence matrix */
struct incidence_matrix {
  struct incidence_col *cols;
  int maxb;
  int b;     /* the number of blocks */

};

/* given the number of points, this function sets up the default bounds */
int set_default_bounds(struct bounds*, int);

/* initialize the backtracking by passing in the number of points */
int init_backtrack(int, struct bounds*);
void cleanup_backtrack(void);

/* begin backtracking */
/* Returning false from the complete-design callback stops the search. */
int backtrack(bool (*)(struct incidence_matrix*, long, void*),
#ifdef DEBUG
	      void (*)(struct incidence_matrix*, void*),
#endif
	      int*,
	      void*);

#endif
