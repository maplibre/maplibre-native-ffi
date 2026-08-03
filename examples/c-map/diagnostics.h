// The diagnostics module: the process-global MapLibre log callback and the
// helper that reports a failed C API call with its thread-local diagnostic.

#ifndef C_MAP_DIAGNOSTICS_H
#define C_MAP_DIAGNOSTICS_H

#include <maplibre_native_c.h>

/// Reports a failed C API call. Called on the thread that made the call,
/// before its next C API call, while the thread-local diagnostic still
/// belongs to the failure.
void diagnostics_log_status(const char* message, mln_status status);

/// The mln_log_callback this example installs at startup.
uint32_t diagnostics_log_record(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
);

#endif  // C_MAP_DIAGNOSTICS_H
