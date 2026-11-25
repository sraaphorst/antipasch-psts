#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef ASSERTIONS
#include <assert.h>
#endif

#include "common.h"
#include "stats.h"

/* declare the stats structures */

/* the number of points */
static int p;

/* the number of partial designs generated */
static long pd;

/* the number of complete designs generated */
static long cd;

/* the number of canonicity checks by row */
static long *canoncheckrow;

/* the number of isomorphisms rejected by row */
static long *isorejrow;

/* the number of partial designs checked by the canonicity check
   - this serves as a better approximation of time spent doing
   canonicity checks */
static long canonpd;

/* the time at which the program was started and ended */
static clock_t start;
static clock_t stop;

int init_stats(int points)
{
  int i;

  p = points;
  pd = 0;
  cd = 0;
  canonpd = 0;
  canoncheckrow = 0;
  isorejrow = 0;

  if (!(canoncheckrow = calloc(p, sizeof(long)))) {
    cleanup_stats();
    return FALSE;
  }

  if (!(isorejrow = calloc(p, sizeof(long)))) {
    cleanup_stats();
    return FALSE;
  }

  for (i=0; i < p; ++i)
    canoncheckrow[i] = isorejrow[i] = 0;

  return TRUE;
}

void cleanup_stats(void)
{
  if (canoncheckrow)
    free(canoncheckrow);
  if (isorejrow)
    free(isorejrow);
  canoncheckrow = NULL;
  isorejrow = NULL;

}

void stats_record_partial(void)
{
  ++pd;
}

void stats_record_complete(void)
{
  ++cd;
}

void stats_record_canonicity_check(int point)
{
#ifdef ASSERTIONS
  assert(point >= 0);
  assert(point < p);
#endif

  ++canoncheckrow[point];
}

void stats_record_isomorphism_rejection(int point)
{
#ifdef ASSERTIONS
  assert(point >= 0);
  assert(point < p);
#endif

  ++isorejrow[point];
}

void stats_start_timer(void)
{
  start = clock();
}

void stats_stop_timer(void)
{
  stop = clock();
}

double stats_get_time_taken(void)
{
  if (start == (clock_t)-1 || stop == (clock_t)-1)
    return 0.0;
  return ((double)stop - (double)start) / CLOCKS_PER_SEC;
}

void stats_record_canon_pd(void)
{
  ++canonpd;
}

long stats_get_canon_pd(void)
{
  return canonpd;
}

long stats_get_num_partial(void)
{
  return pd;
}

long stats_get_num_complete(void)
{
  return cd;
}

void stats_display_isomorphism_rejection(FILE *f)
{
  int i;
  double rejrate;

  fprintf(f, "Isomorphism rejection information:\n");
  for (i=0; i < p; ++i) {
    rejrate = (canoncheckrow[i]
	       ? ((double) isorejrow[i]) / ((double) canoncheckrow[i]) * 100
	       : 0);
    fprintf(f, "\tAt row %2d, rejected: %8ld  checked: %8ld (%0.2f%% rejection)\n",
	    i, isorejrow[i], canoncheckrow[i], rejrate);
  }
}
