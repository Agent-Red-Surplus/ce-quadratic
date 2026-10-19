#ifndef FACTORING_MATH_H
#define FACTORING_MATH_H
#include <stdint.h>
#include <stdbool.h>
#define FACTOR_MAX_DEGREE 12
#define FACTOR_MAX_FACTORS 14
#define FACTOR_INTERMEDIATE_CAP 1000000000LL
typedef struct {int degree;int64_t c[FACTOR_MAX_DEGREE+1];} factoring_poly_t;
typedef struct {
    factoring_poly_t factor[FACTOR_MAX_FACTORS];unsigned count;
    int64_t scalar;unsigned long work;
    bool partial,limited,cancelled,verified,valid_input;
} factoring_result_t;
/* Static result storage, valid until the next call. Not reentrant.
   Coefficient c[i] multiplies x^i; input coefficients must be in [-1000,1000]. */
const factoring_result_t *factoring_solve(const factoring_poly_t *input,bool (*cancel)(void));
#endif
