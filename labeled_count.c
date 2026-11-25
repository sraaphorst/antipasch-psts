#include <inttypes.h>
#include <stdlib.h>

#include "common.h"
#include "labeled_count.h"

#define COUNT_BASE UINT32_C(1000000000)

static bool reserve(struct labeled_count *count, size_t size)
{
  if (size <= count->capacity)
    return true;
  if (size > SIZE_MAX / sizeof *count->digits)
    return false;
  uint32_t *digits = realloc(count->digits, size * sizeof *digits);
  if (!digits)
    return false;
  count->digits = digits;
  count->capacity = size;
  return true;
}

static uint64_t gcd(uint64_t a, uint64_t b)
{
  while (b) {
    uint64_t remainder = a % b;
    a = b;
    b = remainder;
  }
  return a;
}

enum labeled_count_result labeled_count_orbit(struct labeled_count *count, int points, uint64_t automsize)
{
  if (points < 0 || points > POINT_BITS || automsize == 0)
    return LABELED_COUNT_INVALID;

  /* Cancel the denominator against the factors of points! first. A remaining
     denominator proves a Lagrange violation; no large-integer division or
     overflowing factorial is needed. */
  uint32_t factors[POINT_BITS];
  uint64_t denominator = automsize;
  for (int i = 1; i <= points; ++i) {
    uint64_t divisor = gcd(denominator, (uint64_t)i);
    factors[i - 1] = (uint32_t)((uint64_t)i / divisor);
    denominator /= divisor;
  }
  if (denominator != 1)
    return LABELED_COUNT_INVALID;

  if (!reserve(count, 1))
    return LABELED_COUNT_NO_MEMORY;
  count->size = 1;
  count->digits[0] = 1;
  for (int i = 0; i < points; ++i) {
    if (!reserve(count, count->size + 1))
      return LABELED_COUNT_NO_MEMORY;
    uint64_t carry = 0;
    for (size_t j = 0; j < count->size; ++j) {
      uint64_t product = (uint64_t)count->digits[j] * factors[i] + carry;
      count->digits[j] = (uint32_t)(product % COUNT_BASE);
      carry = product / COUNT_BASE;
    }
    if (carry)
      count->digits[count->size++] = (uint32_t)carry;
  }
  return LABELED_COUNT_OK;
}

bool labeled_count_add(struct labeled_count *total, const struct labeled_count *term)
{
  size_t size = total->size > term->size ? total->size : term->size;
  if (size == SIZE_MAX || !reserve(total, size + 1))
    return false;
  uint64_t carry = 0;
  for (size_t i = 0; i < size; ++i) {
    uint64_t sum = carry;
    if (i < total->size) sum += total->digits[i];
    if (i < term->size) sum += term->digits[i];
    total->digits[i] = (uint32_t)(sum % COUNT_BASE);
    carry = sum / COUNT_BASE;
  }
  total->size = size;
  if (carry)
    total->digits[total->size++] = (uint32_t)carry;
  return true;
}

void labeled_count_print(FILE *out, const struct labeled_count *count)
{
  if (!count->size) {
    fputc('0', out);
    return;
  }
  fprintf(out, "%" PRIu32, count->digits[count->size - 1]);
  for (size_t i = count->size - 1; i > 0; --i)
    fprintf(out, "%09" PRIu32, count->digits[i - 1]);
}

void labeled_count_free(struct labeled_count *count)
{
  free(count->digits);
  *count = (struct labeled_count){0};
}
