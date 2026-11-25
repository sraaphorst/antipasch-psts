#ifndef MITRES_H
#define MITRES_H

#include <stdint.h>
#include "backtrack.h"

/* Count distinct five-block mitres in a valid PSTS with complete triples.
   Point labels must be in [0, POINT_BITS). */
uint64_t count_mitres(const struct incidence_matrix *design);

#endif
