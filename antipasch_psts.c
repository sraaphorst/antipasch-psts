/* Local anti-Pasch partial Steiner triple system generator. */
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backtrack.h"
#include "labeled_count.h"
#include "lexicographic.h"
#include "mitres.h"
#ifdef STATS
#include "stats.h"
#endif

struct options {
  struct bounds bounds;
  const char *canstr;
  bool intro;
#ifdef DEBUG
  bool partial, complete, zeros;
#endif
#ifdef STATS
  bool stats, time, efficiency;
#endif
};

static void usage(FILE *out, const char *program)
{
  fprintf(out, "Usage: %s [options] number_of_points\n"
          "Generate anti-Pasch PSTS locally (3 through %d points).\n"
          "  -r N, -R N  Lower/upper bounds on replication\n"
          "  -b N, -B N  Lower/upper bounds on blocks\n"
          "  -C RANGE    Rows to check for canonicity (e.g. -2,4,6,9-10,12-)\n"
          "              Row numbers are zero-based; the final row is always checked.\n"
          "  -i          Print initial bounds\n"
#ifdef STATS
          "  -s          Print statistics\n"
          "  -t          Print CPU time\n"
          "  -e          Print canonicity processing count\n"
#endif
#ifdef DEBUG
          "  -p          Print partial design matrices\n"
          "  -c          Print complete design matrices\n"
          "  -q          Include zero rows in matrices\n"
#endif
          "  -h, --help  Show help\n", program, POINT_BITS);
}

/* Reject empty input, signs, trailing text and overflow. */
static bool parse_number(const char *text, int *value)
{
  char *end;
  long number;
  if (!text || *text < '0' || *text > '9')
    return false;
  errno = 0;
  number = strtol(text, &end, 10);
  if (errno == ERANGE || *end || number > INT_MAX)
    return false;
  *value = (int)number;
  return true;
}

static bool range_number(const char **cursor, int points, int *value)
{
  const char *s = *cursor;
  int number = 0;
  if (*s < '0' || *s > '9')
    return false;
  do {
    /* All valid row numbers are smaller than POINT_BITS. */
    number = number * 10 + (*s++ - '0');
    if (number >= points)
      return false;
  } while (*s >= '0' && *s <= '9');
  *value = number;
  *cursor = s;
  return true;
}

static bool canonicity_rows(const char *s, int points, int *rows)
{
  if (!s) {
    for (int i = 3; i < points; ++i)
      rows[i] = 1;
  } else {
    if (!*s)
      return false;
    while (*s) {
      int lo = 0, hi = points - 1;
      if (*s != '-' && !range_number(&s, points, &lo))
        return false;
      if (*s == '-') {
        ++s;
        if (*s && *s != ',' && !range_number(&s, points, &hi))
          return false;
      } else {
        hi = lo;
      }
      if (lo > hi || (*s && *s != ','))
        return false;
      for (int i = lo; i <= hi; ++i)
        rows[i] = 1;
      if (*s == ',' && !*++s)
        return false;
    }
  }
  rows[points - 1] = 1;
  return true;
}

static bool arguments(int argc, char **argv, struct options *options, int *points)
{
  bool positional = false, end_options = false;
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (!end_options && strcmp(arg, "--") == 0) {
      end_options = true;
      continue;
    }
    if (!end_options && arg[0] == '-' && arg[1]) {
      for (size_t j = 1; arg[j]; ++j) {
        char option = arg[j];
        int *bound = NULL;
        switch (option) {
        case 'r': bound = &options->bounds.lor; break;
        case 'R': bound = &options->bounds.upr; break;
        case 'b': bound = &options->bounds.lob; break;
        case 'B': bound = &options->bounds.upb; break;
        case 'C': break;
        case 'i': options->intro = true; continue;
#ifdef STATS
        case 's': options->stats = true; continue;
        case 't': options->time = true; continue;
        case 'e': options->efficiency = true; continue;
#endif
#ifdef DEBUG
        case 'p': options->partial = true; continue;
        case 'c': options->complete = true; continue;
        case 'q': options->zeros = true; continue;
#endif
        default:
          fprintf(stderr, "Unknown option: -%c\n", option);
          return false;
        }
        const char *value = arg[j + 1] ? arg + j + 1 : (i + 1 < argc ? argv[++i] : NULL);
        if (!value || (bound && !parse_number(value, bound))) {
          fprintf(stderr, "Invalid or missing value for -%c\n", option);
          return false;
        }
        if (option == 'C')
          options->canstr = value;
        break;
      }
    } else {
      if (positional || !parse_number(arg, points)) {
        fprintf(stderr, "Expected one integer number of points\n");
        return false;
      }
      positional = true;
    }
  }
  return positional;
}

#ifdef DEBUG
static void display_matrix(const struct incidence_matrix *matrix, const struct options *options, int points)
{
  int columns = 0;
  while (columns < matrix->maxb && matrix->cols[columns].c)
    ++columns;
  for (int row = 0; row < points; ++row) {
    bool next_row = false;
    for (int col = 0; col < columns; ++col) {
      printf("%d", gbit(matrix->cols[col].v, row));
      if (row + 1 < points && gbit(matrix->cols[col].v, row + 1))
        next_row = true;
    }
    putchar('\n');
    if (!options->zeros && !next_row)
      break;
  }
}
#endif

struct context {
  struct options *options;
  int points;
  struct labeled_count total;
};

static bool complete_design(struct incidence_matrix *matrix, long automsize, void *userdata)
{
  struct context *context = userdata;
  struct labeled_count labeled = {0};
  enum labeled_count_result result = labeled_count_orbit(
      &labeled, context->points, automsize > 0 ? (uint64_t)automsize : 0);
  if (result == LABELED_COUNT_INVALID) {
    fprintf(stderr, "Invalid automorphism group size %ld: it must be positive and divide %d!\n",
            automsize, context->points);
    return false;
  }
  if (result == LABELED_COUNT_NO_MEMORY || !labeled_count_add(&context->total, &labeled)) {
    fprintf(stderr, "Out of memory while counting labeled designs\n");
    labeled_count_free(&labeled);
    return false;
  }
#ifdef DEBUG
  if (context->options->complete)
    display_matrix(matrix, context->options, context->points);
#endif
  printf("Design:");
  for (int i = 0; i < matrix->b; ++i)
    printf(" {%d, %d, %d}", matrix->cols[i].n[0], matrix->cols[i].n[1], matrix->cols[i].n[2]);
  printf(" (automsize: %ld, mitres: %" PRIu64 ", #DistDesigns: ", automsize, count_mitres(matrix));
  labeled_count_print(stdout, &labeled);
  puts(")");
  labeled_count_free(&labeled);
  return true;
}

#ifdef DEBUG
static void partial_design(struct incidence_matrix *matrix, void *userdata)
{
  struct context *context = userdata;
  puts("\nPartial Design:");
  display_matrix(matrix, context->options, context->points);
}
#endif

int main(int argc, char **argv)
{
  struct options options = {.bounds = {-1, -1, -1, -1}};
  struct context context = {.options = &options};
  struct bounds bounds;
  int points = 0;
  int result = EXIT_FAILURE;
  int *cancheck = NULL;

  if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
    usage(stdout, argv[0]);
    return EXIT_SUCCESS;
  }
  if (!arguments(argc, argv, &options, &points)) {
    usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }
  if (points < 3 || points > POINT_BITS) {
    fprintf(stderr, "Points must be between 3 and %d\n", POINT_BITS);
    return EXIT_FAILURE;
  }
  set_default_bounds(&bounds, points);
  if (options.bounds.lor >= 0) bounds.lor = options.bounds.lor;
  if (options.bounds.upr >= 0) bounds.upr = options.bounds.upr;
  if (options.bounds.lob >= 0) bounds.lob = options.bounds.lob;
  if (options.bounds.upb >= 0) bounds.upb = options.bounds.upb;
  /* The row generator requires each point to occur in at least one block. */
  if (bounds.lor < 1 || bounds.upr < 1 || bounds.upb < 1 ||
      bounds.lor > bounds.upr || bounds.lob > bounds.upb ||
      bounds.upr > (points - 1) / 2 || bounds.upb > points * (points - 1) / 6) {
    fprintf(stderr, "Invalid bounds for %d points\n", points);
    return EXIT_FAILURE;
  }

  cancheck = calloc((size_t)points, sizeof *cancheck);
  if (!cancheck) {
    fprintf(stderr, "Out of memory\n");
    goto cleanup;
  }
  if (!canonicity_rows(options.canstr, points, cancheck)) {
    fprintf(stderr, "Invalid canonicity row range (valid rows: 0 through %d)\n", points - 1);
    goto cleanup;
  }
#ifdef STATS
  if (!init_stats(points)) {
    fprintf(stderr, "Could not initialize statistics\n");
    goto cleanup;
  }
#endif
  if (!init_lexicographic(points) || !init_backtrack(points, &bounds)) {
    fprintf(stderr, "Could not initialize generator\n");
    goto cleanup;
  }
  if (options.intro)
    printf("For v=%d, the bounds are:\n\tUpper r: %d\t  Lower r: %d\n"
           "\tUpper b: %d\t  Lower b: %d\n", points, bounds.upr, bounds.lor, bounds.upb, bounds.lob);
#ifdef STATS
  if (options.time) stats_start_timer();
#endif
  context.points = points;
  if (!backtrack(complete_design,
#ifdef DEBUG
            options.partial ? partial_design : NULL,
#endif
            cancheck, &context))
    goto cleanup;
#ifdef STATS
  if (options.time) stats_stop_timer();
  if (options.stats) {
    printf("\nThe program generated:\n\t%ld partial designs\n\t%ld complete designs\n",
           stats_get_num_partial(), stats_get_num_complete());
    stats_display_isomorphism_rejection(stdout);
  }
  if (options.efficiency)
    printf("Performed %ld partial canonicity checks.\n", stats_get_canon_pd());
  if (options.time)
    printf("The program executed for %0.2f s (CPU time).\n", stats_get_time_taken());
#endif
  printf("Total #DistDesigns: ");
  labeled_count_print(stdout, &context.total);
  putchar('\n');
  result = EXIT_SUCCESS;
cleanup:
  labeled_count_free(&context.total);
  free(cancheck);
  cleanup_backtrack();
  cleanup_lexicographic();
#ifdef STATS
  cleanup_stats();
#endif
  return result;
}
