#include <maplibre_native_c.h>
#include <string.h>

static mln_status add_api_header(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* response
) {
  (void)kind;
  if (strncmp(url, "https://tiles.example.com/", 26) != 0) {
    return MLN_STATUS_OK;
  }
  const char* token = user_data;
  return mln_http_header_transform_response_set(
    response, "Authorization", 13, token, strlen(token)
  );
}

void install_http_headers(mln_runtime runtime, const char* token) {
  const mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = add_api_header,
    .user_data = (void*)token,
  };
  mln_runtime_set_http_header_transform(runtime, &transform);
}
