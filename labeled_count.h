#ifndef LABELED_COUNT_H
#define LABELED_COUNT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* An exact nonnegative integer, stored in base 10^9. Initialize with {0}
   and release with labeled_count_free(); an empty value represents zero. */
struct labeled_count {
  uint32_t *digits;
  size_t size, capacity;
};

enum labeled_count_result {
  LABELED_COUNT_OK,
  LABELED_COUNT_INVALID,
  LABELED_COUNT_NO_MEMORY
};

/* Compute points! / automsize, rejecting a zero or nondividing group size.
   Supports the generator's full point range without factorial overflow. */
enum labeled_count_result labeled_count_orbit(struct labeled_count *, int points, uint64_t automsize);
bool labeled_count_add(struct labeled_count *total, const struct labeled_count *term);
void labeled_count_print(FILE *, const struct labeled_count *);
void labeled_count_free(struct labeled_count *);

#endif
