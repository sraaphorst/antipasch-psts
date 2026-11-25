/* lexicographic.h
 *
 * Functions that deal with converting from blocks to
 * lexicographical numbering, and vice versa.
 */

#ifndef LEXICOGRAPHIC_H
#define LEXICOGRAPHIC_H

/* initialize the lexicographical functions,
   passing in the number of points */
int init_lexicographic(int);

/* clean up the library */
void cleanup_lexicographic(void);

/* convert from three points to a lexicographical numbering */
int p_to_lex(int, int, int);

/* convert from a lexicographical numbering to three points */
void lex_to_p(int, int*, int*, int*);

#endif
