#ifndef QUADRATIC_MATH_H
#define QUADRATIC_MATH_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {int32_t p,q,f,r;bool imaginary,radical;} root_t;
typedef struct {root_t roots[2];unsigned count;bool all_numbers,repeated;} quadratic_result_t;
int32_t quad_abs32(int32_t v);
int32_t quad_gcd32(int32_t a,int32_t b);
root_t quad_rational_root(int32_t p,int32_t q);
/* Integer coefficients in [-1000,1000]; output must not be null. */
void quadratic_solve(int32_t a,int32_t b,int32_t c,quadratic_result_t *out);
#endif
