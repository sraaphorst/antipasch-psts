/* canonicity.h
 *
 * This module provides functionality to perform canonicity
 * testing upon a design.
 */

#ifndef CANONICITY_H
#define CANONICITY_H

#include "backtrack.h"

/* initialization and cleanup routines */
int init_canonicity(int);
void cleanup_canonicity(void);

/* test if any design, partial or complete, is canonical
   requires the last row added, the incidence matrix, and the
   cell structure */
int is_canonical(int, struct incidence_matrix*, int**, int*, long*);

#endif
