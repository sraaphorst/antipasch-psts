/* matrix.h
 *
 * Functions to create and destroy 2 and 3 dimensional matrices.
 */

#ifndef MATRIX_H
#define MATRIX_H

/* Right now, we only make matrices with ints.
   This should be sufficient for our purposes. */

/* create an n * m matrix - return null on failure */
int **matrix_2d(int, int);

/* create an n * m * o matrix - return null on failure */
int ***matrix_3d(int, int, int);

void matrix_free_2d(int**, int, int);

void matrix_free_3d(int***, int, int, int);

#endif
