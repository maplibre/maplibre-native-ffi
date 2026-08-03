#include <stdio.h>

#include "diagnostics.h"

static const char* severity_label(uint32_t severity) {
  switch (severity) {
    case MLN_LOG_SEVERITY_INFO:
      return "info";
    case MLN_LOG_SEVERITY_WARNING:
      return "warning";
    case MLN_LOG_SEVERITY_ERROR:
      return "error";
    default:
      return "unknown";
  }
}

void diagnostics_log_status(const char* message, mln_status status) {
  fprintf(stderr, "%s: status %d\n", message, (int)status);
  const char* diagnostic = mln_thread_last_error_message();
  if (diagnostic != nullptr && diagnostic[0] != '\0') {
    fprintf(stderr, "native diagnostic: %s\n", diagnostic);
  }
}

uint32_t diagnostics_log_record(
  [[maybe_unused]] void* user_data, uint32_t severity,
  [[maybe_unused]] uint32_t event, [[maybe_unused]] int64_t code,
  const char* message
) {
  printf("[%s] %s\n", severity_label(severity), message);
  return 1;
}
