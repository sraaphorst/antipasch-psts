/* stats.h
 *
 * Statistical data for the anti-Pasch PSTS generation
 */

#ifndef STATS_H
#define STATS_H

#include <stdio.h>

/* initialization and destruction routines */
int init_stats(int);
void cleanup_stats(void);

/* record designs */
/* these should only be used for feasible designs, and not
   designs that we are going to toss away because they are
   invalid (out of lex order, isomorphic, containing pasches,
   etc...) */
void stats_record_partial(void);
void stats_record_complete(void);

/* record a call to the canonicity testing algorithm */
void stats_record_canonicity_check(int);

/* record an isomorphism rejection for a row */
void stats_record_isomorphism_rejection(int);

/* timer functions */
void stats_start_timer(void);
void stats_stop_timer(void);
double stats_get_time_taken(void);

/* canonicity checking */
void stats_record_canon_pd(void);
long stats_get_canon_pd(void);

/* accessors */
long stats_get_num_partial(void);
long stats_get_num_complete(void);

/* output */
void stats_display_isomorphism_rejection(FILE*);

#endif
