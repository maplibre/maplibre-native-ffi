// Serving a private app:// URL scheme from host storage.

#include <maplibre_native_c.h>
#include <string.h>

typedef struct asset_store asset_store;

// The host supplies a concurrent lookup. Its bytes remain valid until return.
bool asset_store_find(
  const asset_store* store, const char* url, const uint8_t** out_bytes,
  size_t* out_size
);

static uint32_t serve_bundled_asset(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  // #region match
  const asset_store* store = user_data;
  const uint8_t* bytes = NULL;
  size_t size = 0;

  if (
    strncmp(request->resolved_url, "app://", 6) != 0 ||
    !asset_store_find(store, request->resolved_url, &bytes, &size)
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
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
  // The C API copies these bytes before the call returns.
  mln_resource_request_complete(handle, &response);
  mln_resource_request_release(handle);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
  // #endregion complete
}

// #region install
void install_provider(mln_runtime runtime, asset_store* store) {
  const mln_resource_provider provider = {
    .size = sizeof(provider),
    .callback = serve_bundled_asset,
    .user_data = store,
  };
  uint64_t command_id = 0;
  mln_runtime_set_resource_provider(runtime, &provider, &command_id);
}
// #endregion install
