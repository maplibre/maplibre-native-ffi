// An asset:// style resolves under the runtime's asset path.

#include "support/resources.h"
#include "support/test_support.h"

static void host_string_views_reject_nul_and_null_data_with_a_length(void) {
  const mln_buffer_view invalid[] = {
    MLN_BUFFER_LITERAL("before\0after"),
    {.data = NULL, .size = 1},
  };
  for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]);
       ++index) {
    mln_runtime runtime = MLN_HANDLE_NULL;
    mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
    options.asset_path = invalid[index];
    MLN_TEST_INVALID(mln_runtime_create(&options, &runtime, NULL));
    options.asset_path = (mln_buffer_view){0};
    options.cache_path = invalid[index];
    MLN_TEST_INVALID(mln_runtime_create(&options, &runtime, NULL));
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  }
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]);
       ++index) {
    MLN_TEST_INVALID(mln_map_set_style_url(map, invalid[index], NULL, NULL));
    MLN_TEST_INVALID(
      mln_runtime_merge_offline_regions(runtime, invalid[index], NULL, NULL)
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

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

  mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  options.asset_path = mln_buffer_view_from_cstr(directory);
  mln_runtime runtime = mln_test_create_runtime_with_options(options);
  mln_map map = mln_test_create_map(runtime);
  const size_t url_size = strlen(url);
  url[url_size] = '!';
  url[url_size + 1] = '\0';
  MLN_TEST_AWAIT_OK(mln_map_set_style_url(
    map, (mln_buffer_view){.data = url, .size = url_size},
    &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  (void)remove(path);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(host_string_views_reject_nul_and_null_data_with_a_length);
#if !defined(__EMSCRIPTEN__) && !defined(__OHOS__)
  RUN_TEST(an_asset_style_resolves_under_the_runtime_asset_path);
#endif
}
