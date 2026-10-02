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

void diagnostics_log_status(
  const char* message, mln_status status, const mln_diagnostic* diagnostic
) {
  fprintf(stderr, "%s: status %d\n", message, (int)status);
  if (diagnostic != nullptr && diagnostic->message[0] != '\0') {
    fprintf(stderr, "native diagnostic: %s\n", diagnostic->message);
  }
}

static void log_failed_completion(
  void* user_data, const mln_completion_result* result
) {
  if (result->status == MLN_STATUS_OK) return;
  fprintf(stderr, "%s: status %d\n", (const char*)user_data, result->status);
  if (result->diagnostic.size != 0) {
    fprintf(
      stderr, "native diagnostic: %.*s\n", (int)result->diagnostic.size,
      (const char*)result->diagnostic.data
    );
  }
}

mln_completion diagnostics_completion(const char* message) {
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = log_failed_completion,
    .user_data = (void*)message,
  };
}

uint32_t diagnostics_log_record(
  [[maybe_unused]] void* user_data, uint32_t severity,
  [[maybe_unused]] uint32_t event, [[maybe_unused]] int64_t code,
  const char* message
) {
  printf("[%s] %s\n", severity_label(severity), message);
  return 1;
}
