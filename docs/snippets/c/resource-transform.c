// Attaching an API key to tile requests. A style pulls sprites, glyphs, and
// tiles from whatever origins it names, and the transform sees all of them, so
// match the host you own before adding a credential.

#include <maplibre_native_c.h>
#include <stdio.h>
#include <string.h>

static const char trusted_prefix[] = "https://tiles.example.com/";

static mln_status add_api_key(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  const char* api_key = user_data;
  (void)kind;

  if (strncmp(url, trusted_prefix, sizeof(trusted_prefix) - 1) != 0) {
    return MLN_STATUS_OK;  // Leaving the URL unset keeps the original.
  }

  // The key belongs in the query, so split off any fragment and put it back
  // afterwards. A key appended past '#' never reaches the server.
  const char* fragment = strchr(url, '#');
  const size_t base_length =
    fragment != NULL ? (size_t)(fragment - url) : strlen(url);
  const char* separator = memchr(url, '?', base_length) != NULL ? "&" : "?";

  char rewritten[2048];
  const int written = snprintf(
    rewritten, sizeof(rewritten), "%.*s%skey=%s%s", (int)base_length, url,
    separator, api_key, fragment != NULL ? fragment : ""
  );
  if (written < 0 || (size_t)written >= sizeof(rewritten)) {
    return MLN_STATUS_OK;  // A truncated URL is worse than an unkeyed one.
  }

  return mln_resource_transform_response_set_url(
    out_response, rewritten, (size_t)written
  );
}

void install_transform(mln_runtime* runtime, char* api_key) {
  // The callback runs on network threads, and both it and user_data must stay
  // alive until the transform is cleared or the runtime is destroyed.
  mln_resource_transform transform = {
    .size = sizeof(transform),
    .callback = add_api_key,
    .user_data = api_key,
  };
  mln_runtime_set_resource_transform(runtime, &transform);
}
