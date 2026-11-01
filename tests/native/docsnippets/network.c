// Snippets for the docs site's Network page. They compile with the native
// tests, which never run them.

#include <maplibre_native_c.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char api_key_parameter[] = "key=my-api-key";

static mln_buffer_view with_api_key(
  mln_buffer_view url, char* buffer, size_t capacity
) {
  const size_t key_size = sizeof(api_key_parameter) - 1;
  if (url.size + 1 + key_size > capacity) {
    return url;
  }
  memcpy(buffer, url.data, url.size);
  buffer[url.size] = memchr(url.data, '?', url.size) == NULL ? '?' : '&';
  memcpy(buffer + url.size + 1, api_key_parameter, key_size);
  return (mln_buffer_view){.data = buffer, .size = url.size + 1 + key_size};
}

static mln_buffer_view find_tile(mln_buffer_view url) {
  (void)url;
  return (mln_buffer_view){0};
}

static const char* geojson_for_tile(mln_canonical_tile_id tile) {
  (void)tile;
  return "{\"type\":\"FeatureCollection\",\"features\":[]}";
}

// #region rewrite
static void add_api_key(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_resource_transform_request request
) {
  (void)user_data;
  (void)kind;
  char buffer[4096];
  mln_buffer_view new_url = with_api_key(url, buffer, sizeof(buffer));
  if (
    mln_resource_transform_request_complete(request, new_url, NULL) !=
    MLN_STATUS_OK
  ) {
    // Releasing the request without an answer keeps the old URL.
    mln_resource_transform_request_release(request);
  }
}
// #endregion rewrite

// #region provider
static void serve_tile(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)user_data;
  mln_buffer_view tile = find_tile(request->requested_url);

  mln_resource_response response = {.size = sizeof(mln_resource_response)};
  if (tile.size > 0) {
    response.status = MLN_RESOURCE_RESPONSE_STATUS_OK;
    response.bytes = tile.data;
    response.byte_count = tile.size;
  } else {
    response.status = MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT;
  }

  if (mln_resource_request_complete(handle, &response, NULL) != MLN_STATUS_OK) {
    mln_resource_request_release(handle);
  }
}
// #endregion provider

// #region custom-source
static void fetch_tile(void* user_data, mln_canonical_tile_id tile) {
  const mln_map* map = user_data;
  mln_map_set_custom_geometry_source_tile_data(
    *map, mln_buffer_view_from_cstr("app-data"), tile,
    mln_buffer_view_from_cstr(geojson_for_tile(tile)), NULL, NULL
  );
}
// #endregion custom-source

void mln_doc_network(mln_runtime runtime, mln_map map);

void mln_doc_network(mln_runtime runtime, mln_map map) {
  {
    // #region headers
    const mln_http_header header = {
      .name = mln_buffer_view_from_cstr("Authorization"),
      .value = mln_buffer_view_from_cstr("Bearer my-token"),
    };
    const mln_http_header_rule rule = {
      .size = sizeof(mln_http_header_rule),
      .flags = MLN_URL_MATCH_GLOB,
      .url = mln_buffer_view_from_cstr("https://tiles.example.com/**"),
      .headers = &header,
      .header_count = 1,
    };
    const mln_http_header_transform transform = {
      .size = sizeof(mln_http_header_transform),
      .rules = &rule,
      .rule_count = 1,
    };

    mln_runtime_set_http_header_transform(runtime, &transform, NULL, NULL);
    // #endregion headers
  }

  {
    // #region rewrite
    const mln_resource_transform transform = {
      .size = sizeof(mln_resource_transform),
      .callback = add_api_key,
    };

    mln_runtime_set_resource_transform(runtime, &transform, NULL, NULL);
    // #endregion rewrite
  }

  {
    // #region provider
    const mln_resource_route route = {
      .size = sizeof(mln_resource_route),
      .kinds = MLN_RESOURCE_KIND_FLAG_TILE,
      .flags = MLN_URL_MATCH_GLOB,
      .url = mln_buffer_view_from_cstr("app://tiles/**"),
    };
    const mln_resource_provider provider = {
      .size = sizeof(mln_resource_provider),
      .callback = serve_tile,
      .routes = &route,
      .route_count = 1,
    };

    mln_runtime_set_resource_provider(runtime, &provider, NULL, NULL);
    // #endregion provider
  }

  {
    // #region custom-source
    // The library frees this copy when it removes the source.
    mln_map* source_map = malloc(sizeof(*source_map));
    *source_map = map;

    const mln_custom_geometry_source_options options = {
      .size = sizeof(mln_custom_geometry_source_options),
      .fetch_tile = fetch_tile,
      .user_data = source_map,
      .release_user_data = free,
    };

    mln_map_add_custom_geometry_source(
      map, mln_buffer_view_from_cstr("app-data"), &options, NULL, NULL
    );
    // #endregion custom-source
  }

  // #region status
  mln_network_set_status(MLN_NETWORK_STATUS_OFFLINE, NULL);

  // Later, when the connection returns:
  mln_network_set_status(MLN_NETWORK_STATUS_ONLINE, NULL);
  // #endregion status
}
