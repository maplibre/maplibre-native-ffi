// The part of <math.h> that the binding's C uses, as WebAssembly instructions.

#ifndef MLN_TS_WEB_MATH_H
#define MLN_TS_WEB_MATH_H

static inline double floor(double value) { return __builtin_floor(value); }

#endif
