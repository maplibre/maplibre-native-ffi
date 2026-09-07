// Sending a bearer token to one origin, and to no other host that a style
// names.

#include <maplibre_native_c.h>
#include <string.h>

static const char trusted_prefix[] = "https://tiles.example.com/";
static const char header_name[] = "Authorization";

static mln_status add_authorization(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) {
  // #region match
  const char* token = user_data;
  (void)kind;

  if (strncmp(url, trusted_prefix, sizeof(trusted_prefix) - 1) != 0) {
    return MLN_STATUS_OK;  // Every other origin gets no header.
  }
  // #endregion match

  // #region set
  // The helper copies the name and the value before it returns.
  return mln_http_header_transform_response_set(
    out_response, header_name, sizeof(header_name) - 1, token, strlen(token)
  );
  // #endregion set
}

// #region install
mln_status install_header_transform(
  mln_runtime runtime, char* token, const mln_completion* completion
) {
  mln_http_header_transform transform = {
    .size = sizeof(transform),
    .callback = add_authorization,
    .user_data = token,
  };
  return mln_runtime_set_http_header_transform(runtime, &transform, completion);
}
// #endregion install
