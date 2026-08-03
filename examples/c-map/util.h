// Small helpers shared across the example's translation units.

#ifndef C_MAP_UTIL_H
#define C_MAP_UTIL_H

#include <threads.h>
#include <time.h>

#include "types.h"

/// Returns from the enclosing function when a fallible step fails, so setup
/// sequences read as straight lines.
#define MAP_TRY(expr)                    \
  do {                                   \
    const app_error try_error_ = (expr); \
    if (try_error_ != APP_OK) {          \
      return try_error_;                 \
    }                                    \
  } while (0)

static inline void sleep_milliseconds(long milliseconds) {
  thrd_sleep(
    &(struct timespec){.tv_nsec = milliseconds * 1000 * 1000}, nullptr
  );
}

#endif  // C_MAP_UTIL_H
