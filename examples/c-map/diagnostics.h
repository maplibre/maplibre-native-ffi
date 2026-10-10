// The process-global MapLibre log callback and the helpers that report a
// failed C API call with its diagnostic.

#ifndef C_MAP_DIAGNOSTICS_H
#define C_MAP_DIAGNOSTICS_H

#include <maplibre_native_c.h>

/// Reports a failed C API call with the diagnostic it wrote, or with none for a
/// status that arrived through a completion.
void diagnostics_log_status(
  const char* message, mln_status status, const mln_diagnostic* diagnostic
);

/// A completion for work whose result nothing waits on. It reports a failed
/// terminal status, prefixed with message, which must outlive the work.
mln_completion diagnostics_completion(const char* message);

/// The mln_log_callback this example installs at startup.
uint32_t diagnostics_log_record(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
);

#endif  // C_MAP_DIAGNOSTICS_H
