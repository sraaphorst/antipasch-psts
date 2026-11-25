#include "mitres.h"

uint64_t count_mitres(const struct incidence_matrix *design)
{
  /* Store the third point plus one, leaving zero for absent pairs. */
  unsigned char third[POINT_BITS][POINT_BITS] = {{0}};
  uint64_t count = 0;

  for (int block = 0; block < design->b; ++block) {
    const int *points = design->cols[block].n;
    for (int i = 0; i < 3; ++i) {
      int a = points[i], b = points[(i + 1) % 3];
      third[a][b] = third[b][a] = (unsigned char)(points[(i + 2) % 3] + 1);
    }
  }

  for (int centre = 0; centre < POINT_BITS; ++centre) {
    int spokes[POINT_BITS / 2][2];
    int size = 0;
    for (int block = 0; block < design->b; ++block) {
      const int *points = design->cols[block].n;
      for (int i = 0; i < 3; ++i) {
        if (points[i] == centre) {
          spokes[size][0] = points[(i + 1) % 3];
          spokes[size][1] = points[(i + 2) % 3];
          ++size;
          break;
        }
      }
    }

    /* A mitre has three spokes through its unique degree-three centre,
       and two transversal blocks joining their six other points.
       Choose unordered spokes, then fix the first endpoint to avoid
       counting a transversal pair again in the opposite order. */
    for (int i = 0; i < size; ++i)
      for (int j = i + 1; j < size; ++j)
        for (int k = j + 1; k < size; ++k)
          for (int x = 0; x < 2; ++x)
            for (int y = 0; y < 2; ++y)
              if (third[spokes[i][0]][spokes[j][x]] == spokes[k][y] + 1 &&
                  third[spokes[i][1]][spokes[j][1 - x]] == spokes[k][1 - y] + 1)
                ++count;
  }
  return count;
}
