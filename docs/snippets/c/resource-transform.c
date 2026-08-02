// Attaching an API key to the requests that reach one origin.

#include <maplibre_native_c.h>
#include <string.h>

static const char trusted_prefix[] = "https://tiles.example.com/";

// The host supplies this function to add a query parameter before any fragment.
bool build_keyed_url(
  const char* url, const char* key, char* out, size_t out_size
);

static mln_status add_api_key(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  // #region match
  const char* api_key = user_data;
  (void)kind;

  if (strncmp(url, trusted_prefix, sizeof(trusted_prefix) - 1) != 0) {
    return MLN_STATUS_OK;  // An empty response preserves the original URL.
  }
  // #endregion match

  // #region rewrite
  char rewritten[2048];
  if (!build_keyed_url(url, api_key, rewritten, sizeof(rewritten))) {
    return MLN_STATUS_OK;
  }

  // The helper copies the URL before rewritten leaves scope.
  return mln_resource_transform_response_set_url(
    out_response, rewritten, strlen(rewritten)
  );
  // #endregion rewrite
}

// #region install
void install_transform(mln_runtime runtime, char* api_key) {
  mln_resource_transform transform = {
    .size = sizeof(transform),
    .callback = add_api_key,
    .user_data = api_key,
  };
  mln_runtime_set_resource_transform(runtime, &transform);
}
// #endregion install
