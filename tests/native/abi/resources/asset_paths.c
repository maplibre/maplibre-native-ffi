// An asset:// style resolves under the runtime's asset path.

#include "support/resources.h"
#include "support/test_support.h"

// The browser and OpenHarmony have no asset directory on disk. Android reads
// its packaged assets only for a path under /android_asset/, so a directory on
// disk resolves there as it does on the desktop and simulator targets.
#if !defined(__EMSCRIPTEN__) && !defined(__OHOS__)

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
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  (void)remove(path);
}

#endif

MLN_TEST_GROUP {
#if !defined(__EMSCRIPTEN__) && !defined(__OHOS__)
  RUN_TEST(an_asset_style_resolves_under_the_runtime_asset_path);
#endif
}
