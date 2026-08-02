// Taking MapLibre Native's log records into the host's own logger.

#include <maplibre_native_c.h>
#include <stdint.h>

// Your logging system. Called from MapLibre's threads, so it must be safe to
// call from several at once.
extern void host_log(int severity, const char* category, const char* message);

static const char* category_name(uint32_t event) {
  switch (event) {
    case MLN_LOG_EVENT_PARSE_STYLE:
      return "style";
    case MLN_LOG_EVENT_PARSE_TILE:
      return "tile";
    case MLN_LOG_EVENT_HTTP_REQUEST:
      return "http";
    case MLN_LOG_EVENT_DATABASE:
      return "database";
    case MLN_LOG_EVENT_RENDER:
      return "render";
    default:
      return "general";
  }
}

// #region callback
static uint32_t forward_to_host(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) {
  (void)user_data;
  (void)code;

  host_log((int)severity, category_name(event), message);

  // Non-zero consumes the record. Return zero instead to let MapLibre's own
  // platform logger print it as well.
  return 1;
}
// #endregion callback

mln_status capture_logs(void) {
  // #region install
  // The callback is process-global rather than per-runtime, and it is stored
  // by reference, so it must outlive every runtime in the process.
  return mln_log_set_callback(forward_to_host, NULL);
  // #endregion install
}

mln_status make_every_record_synchronous(void) {
  // #region async
  // Errors are already synchronous by default. Clearing the other bits orders
  // every record against the call that produced it, at the cost of blocking
  // MapLibre's threads on the host logger.
  return mln_log_set_async_severity_mask(0);
  // #endregion async
}
