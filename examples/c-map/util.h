// Small helpers shared across the example's translation units.

#ifndef C_MAP_UTIL_H
#define C_MAP_UTIL_H

#include "types.h"

/// Returns from the enclosing function when a fallible step fails.
#define MAP_TRY(expr)                    \
  do {                                   \
    const app_error try_error_ = (expr); \
    if (try_error_ != APP_OK) {          \
      return try_error_;                 \
    }                                    \
  } while (0)

#endif  // C_MAP_UTIL_H
