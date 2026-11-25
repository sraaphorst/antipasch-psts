#include <stdio.h>
#include <string.h>
#include "common.h"
#include "labeled_count.h"
#include "lexicographic.h"
#include "matrix.h"
#include "mitres.h"

#define CHECK(condition) do { if (!(condition)) { \
  fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #condition); \
  return EXIT_FAILURE; } } while (0)

static bool count_equals(const struct labeled_count *count, const char *expected)
{
  FILE *file = tmpfile();
  if (!file)
    return false;
  labeled_count_print(file, count);
  rewind(file);
  char text[256];
  size_t size = fread(text, 1, sizeof text - 1, file);
  bool ok = !ferror(file);
  text[size] = '\0';
  fclose(file);
  return ok && strcmp(text, expected) == 0;
}

int main(void)
{
  struct labeled_count orbit = {0}, total = {0};
  CHECK(count_equals(&total, "0"));
  CHECK(labeled_count_orbit(&orbit, 9, 432) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "840"));
  CHECK(labeled_count_add(&total, &orbit));
  CHECK(labeled_count_orbit(&orbit, 11, 8) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "4989600"));
  CHECK(labeled_count_add(&total, &orbit));
  CHECK(count_equals(&total, "4990440"));
  CHECK(labeled_count_orbit(&orbit, 3, 0) == LABELED_COUNT_INVALID);
  CHECK(labeled_count_orbit(&orbit, 3, 4) == LABELED_COUNT_INVALID);
  CHECK(labeled_count_orbit(&orbit, 3, 12) == LABELED_COUNT_INVALID);
  CHECK(labeled_count_orbit(&orbit, 64, UINT64_MAX) == LABELED_COUNT_INVALID);
  CHECK(labeled_count_orbit(&orbit, 65, 1) == LABELED_COUNT_INVALID);
  CHECK(labeled_count_orbit(&orbit, -1, 1) == LABELED_COUNT_INVALID);
  CHECK(labeled_count_orbit(&orbit, 0, 1) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "1"));
  CHECK(labeled_count_orbit(&orbit, 20, UINT64_C(2432902008176640000)) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "1"));
  CHECK(labeled_count_orbit(&orbit, 20, 1) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "2432902008176640000"));
  CHECK(labeled_count_orbit(&orbit, 21, 1) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "51090942171709440000"));
  CHECK(labeled_count_orbit(&orbit, 64, UINT64_C(9223372036854775808)) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "13757108753595648665519665029568345104465749222289382342659100341796875"));
  CHECK(labeled_count_orbit(&orbit, 64, 1) == LABELED_COUNT_OK);
  CHECK(count_equals(&orbit, "126886932185884164103433389335161480802865516174545192198801894375214704230400000000000000"));
  labeled_count_free(&total);
  CHECK(labeled_count_add(&total, &orbit));
  CHECK(labeled_count_add(&total, &total));
  CHECK(labeled_count_add(&total, &orbit));
  CHECK(count_equals(&total, "380660796557652492310300168005484442408596548523635576596405683125644112691200000000000000"));
  labeled_count_free(&orbit);
  labeled_count_free(&total);
  labeled_count_free(&total);

  struct incidence_col blocks[10] = {
    {.n = {0, 1, 2}, .c = 3}, {.n = {0, 3, 4}, .c = 3},
    {.n = {0, 5, 6}, .c = 3}, {.n = {1, 3, 5}, .c = 3},
    {.n = {2, 4, 6}, .c = 3}
  };
  struct incidence_matrix design = {.cols = blocks, .maxb = 10, .b = 0};
  CHECK(count_mitres(&design) == 0);
  design.b = 4;
  CHECK(count_mitres(&design) == 0);
  design.b = 5;
  CHECK(count_mitres(&design) == 1);
  /* Relabel and reverse both block and point order, including label 63. */
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 3; ++j)
      blocks[9 - i].n[2 - j] = POINT_BITS - 1 - blocks[i].n[j];
  design.b = 10;
  CHECK(count_mitres(&design) == 2);

  point_bits bits = 0;
  for (int i = 0; i < POINT_BITS; ++i) {
    sbit(bits, i, 1);
    CHECK(gbit(bits, i));
  }
  CHECK(bits == UINT64_MAX);
  for (int i = 0; i < POINT_BITS; ++i) {
    sbit(bits, i, 0);
    CHECK(!gbit(bits, i));
  }
  CHECK(bits == 0);
  matrix_free_2d(NULL, 3, 3);
  matrix_free_3d(NULL, 3, 3, 3);
  CHECK(matrix_2d(-1, 1) == NULL);
  CHECK(matrix_3d(1, 0, 1) == NULL);
  cleanup_lexicographic();
  for (int points = 3; points <= POINT_BITS; points += (points == 3 ? 6 : 55)) {
    CHECK(init_lexicographic(points));
    int index = 0;
    for (int a = 0; a < points; ++a)
      for (int b = a + 1; b < points; ++b)
        for (int c = b + 1; c < points; ++c, ++index) {
          int x, y, z;
          CHECK(p_to_lex(a, b, c) == index);
          CHECK(p_to_lex(a, c, b) == index);
          CHECK(p_to_lex(b, a, c) == index);
          CHECK(p_to_lex(b, c, a) == index);
          CHECK(p_to_lex(c, a, b) == index);
          CHECK(p_to_lex(c, b, a) == index);
          lex_to_p(index, &x, &y, &z);
          CHECK(x == a && y == b && z == c);
        }
    cleanup_lexicographic();
    cleanup_lexicographic();
  }
  return EXIT_SUCCESS;
}
