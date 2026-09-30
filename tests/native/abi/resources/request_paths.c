// Where requests go: an asset:// style resolves under the runtime's asset
// path, and a network image request reaches both transforms marked as an
// image.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "support/harness.h"
#include "support/resources.h"
#include "support/test_support.h"
#include "unity.h"

#if !defined(__EMSCRIPTEN__)
#include "support/http_server.h"
#endif

// Android resolves asset:// against the application's packaged assets, which
// need mln_android_init, and the browser and OpenHarmony have no asset
// directory on disk, so the case runs on the desktop and simulator targets.
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__) && !defined(__OHOS__)

static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// An asset:// URL names a file relative to the directory the runtime's
// asset_path option gives.
static void an_asset_style_resolves_under_the_runtime_asset_path(void) {
  char path[1024];
  mln_test_temp_path("asset-style.json", path, sizeof(path));
  FILE* file = fopen(path, "wb");
  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_size_t(
    sizeof(empty_style_json) - 1,
    fwrite(empty_style_json, 1, sizeof(empty_style_json) - 1, file)
  );
  TEST_ASSERT_EQUAL_INT(0, fclose(file));

  char directory[1024];
  (void)snprintf(directory, sizeof(directory), "%s", path);
  char* separator = strrchr(directory, '/');
  char* backslash = strrchr(directory, '\\');
  if (backslash != NULL && (separator == NULL || backslash > separator)) {
    separator = backslash;
  }
  TEST_ASSERT_NOT_NULL(separator);
  char url[1100];
  (void)snprintf(url, sizeof(url), "asset://%s", separator + 1);
  *separator = '\0';

  mln_runtime_options options = mln_runtime_options_default();
  options.asset_path = directory;
  mln_runtime runtime = mln_test_create_runtime_with_options(options);
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  (void)remove(path);
}

#endif

// The browser has no loopback server, and the browser and OpenHarmony
// transports refuse the header transform.
#if !defined(__EMSCRIPTEN__) && !defined(__OHOS__)

typedef struct kind_probe {
  atomic_bool saw_image;
} kind_probe;

static mln_status note_transform_kind(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  (void)url;
  (void)out_response;
  kind_probe* probe = user_data;
  if (kind == MLN_RESOURCE_KIND_IMAGE) mln_test_flag_set(&probe->saw_image);
  return MLN_STATUS_OK;
}

static mln_status note_header_kind(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) {
  (void)url;
  (void)out_response;
  kind_probe* probe = user_data;
  if (kind == MLN_RESOURCE_KIND_IMAGE) mln_test_flag_set(&probe->saw_image);
  return MLN_STATUS_OK;
}

// An image source's URL is fetched as an image request, and both the URL
// transform and the header transform see it as one.
static void a_network_image_request_reaches_both_transforms_as_an_image(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/image.png", .status = 404},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char image_url[256];
  mln_test_http_server_url(server, "/image.png", image_url, sizeof(image_url));
  char style[512];
  (void)snprintf(
    style, sizeof(style),
    "{\"version\":8,\"sources\":{\"picture\":{\"type\":\"image\",\"url\":"
    "\"%s\",\"coordinates\":[[0,1],[1,1],[1,0],[0,0]]}},\"layers\":[]}",
    image_url
  );

  kind_probe transform_probe;
  kind_probe header_probe;
  atomic_init(&transform_probe.saw_image, false);
  atomic_init(&header_probe.saw_image, false);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = note_transform_kind,
    .user_data = &transform_probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_set_resource_transform(runtime, &transform)
  );
  const mln_http_header_transform header_transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = note_header_kind,
    .user_data = &header_probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_set_http_header_transform(runtime, &header_transform)
  );
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, (mln_buffer_view){.data = style, .size = strlen(style)}
  );
  TEST_ASSERT_TRUE(
    mln_test_http_server_wait_for_requests(server, "/image.png", 1)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&transform_probe.saw_image));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&header_probe.saw_image));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

#endif

MLN_TEST_GROUP {
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__) && !defined(__OHOS__)
  RUN_TEST(an_asset_style_resolves_under_the_runtime_asset_path);
#endif
#if !defined(__EMSCRIPTEN__) && !defined(__OHOS__)
  RUN_TEST(a_network_image_request_reaches_both_transforms_as_an_image);
#endif
}
