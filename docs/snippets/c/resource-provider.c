// Serving a private app:// URL scheme from host storage.

#include <maplibre_native_c.h>
#include <string.h>

typedef struct asset_store asset_store;

// The host supplies a concurrent lookup. Its bytes remain valid until return.
bool asset_store_find(
  const asset_store* store, const char* url, const uint8_t** out_bytes,
  size_t* out_size
);

static void serve_bundled_asset(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  // #region match
  const asset_store* store = user_data;
  const uint8_t* bytes = NULL;
  size_t size = 0;

  // Native ends the URL with a NUL, so it also reads as a C string.
  const char* url = request->resolved_url.data;
  if (
    strncmp(url, "app://", 6) != 0 ||
    !asset_store_find(store, url, &bytes, &size)
  ) {
    // Declining hands the request to the built-in loader.
    if (mln_resource_request_pass_through(handle, NULL) != MLN_STATUS_OK) {
      mln_resource_request_release(handle);
    }
    return;
  }
  // #endregion match

  // #region complete
  const mln_resource_response response = {
    .size = sizeof(response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = bytes,
    .byte_count = size,
  };
  // The C API copies these bytes before the call returns. Completing consumes
  // the handle; a request that was cancelled meanwhile is released instead.
  if (mln_resource_request_complete(handle, &response, NULL) != MLN_STATUS_OK) {
    mln_resource_request_release(handle);
  }
  // #endregion complete
}

// #region cancel
typedef struct pending_fetch pending_fetch;

// The host's I/O pool aborts the fetch. Its completion path then completes the
// request, which consumes the handle, or releases the handle of an aborted
// fetch.
void pending_fetch_abort(pending_fetch* fetch);

static void abort_pending_fetch(void* user_data, mln_resource_request handle) {
  (void)handle;
  pending_fetch_abort(user_data);
}

// Registers the abort hook for a request the provider retained. MapLibre runs
// the hook on one of its threads when the map stops wanting the resource. A
// request that was cancelled before the registration reports that instead.
void watch_for_cancellation(mln_resource_request handle, pending_fetch* fetch) {
  const mln_resource_request_cancel_handler handler = {
    .size = sizeof(handler),
    .callback = abort_pending_fetch,
    .user_data = fetch,
  };
  bool already_cancelled = false;
  if (
    mln_resource_request_set_cancel_callback(
      handle, &handler, &already_cancelled, NULL
    ) == MLN_STATUS_OK &&
    already_cancelled
  ) {
    pending_fetch_abort(fetch);
  }
}
// #endregion cancel

// #region install
mln_status install_provider(
  mln_runtime runtime, asset_store* store, const mln_completion* completion
) {
  const mln_resource_route route = {
    .size = sizeof(mln_resource_route),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = mln_buffer_view_from_cstr("app://**"),
  };
  const mln_resource_provider provider = {
    .size = sizeof(provider),
    .callback = serve_bundled_asset,
    .user_data = store,
    .routes = &route,
    .route_count = 1,
  };
  return mln_runtime_set_resource_provider(
    runtime, &provider, completion, NULL
  );
}
// #endregion install

// #region pass-through
// Passing through consumes the handle. A request that was cancelled first keeps
// its handle for the caller to release.
mln_status resume_native_loading(mln_resource_request handle) {
  return mln_resource_request_pass_through(handle, NULL);
}
// #endregion pass-through
