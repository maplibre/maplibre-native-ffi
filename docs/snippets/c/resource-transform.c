// Attaching an API key to tile requests. A style names sprites, glyphs, and
// tiles on origins you do not control, and the transform runs for those too, so
// match your own host before adding the credential.

#include <maplibre_native_c.h>
#include <string.h>

static const char trusted_prefix[] = "https://tiles.example.com/";

// Yours to supply: adds key to url as a query parameter, ahead of any fragment.
bool build_keyed_url(
  const char* url, const char* key, char* out, size_t out_size
);

static mln_status add_api_key(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  const char* api_key = user_data;
  (void)kind;

  if (strncmp(url, trusted_prefix, sizeof(trusted_prefix) - 1) != 0) {
    return MLN_STATUS_OK;  // Leaving the URL unset keeps the original.
  }

  char rewritten[2048];
  if (!build_keyed_url(url, api_key, rewritten, sizeof(rewritten))) {
    return MLN_STATUS_OK;  // No key beats a malformed one.
  }

  return mln_resource_transform_response_set_url(
    out_response, rewritten, strlen(rewritten)
  );
}

void install_transform(mln_runtime* runtime, char* api_key) {
  // MapLibre calls this on its network threads, and both the callback and
  // user_data have to outlive the transform.
  mln_resource_transform transform = {
    .size = sizeof(transform),
    .callback = add_api_key,
    .user_data = api_key,
  };
  mln_runtime_set_resource_transform(runtime, &transform);
}
