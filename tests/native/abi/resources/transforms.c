// The resource transform and the HTTP header transform: what their
// registrations accept, and, against a loopback server, which requests they
// change and how.

#include "support/resources.h"
#include "support/test_support.h"

#if !defined(__EMSCRIPTEN__)
#include "support/http_server.h"
#endif

// The browser and OpenHarmony transports cannot drop a transformed header when
// a redirect changes origin, so they refuse the header transform.
#if defined(__EMSCRIPTEN__) || defined(__OHOS__)
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 0
#else
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 1
#endif

static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static mln_status keep_url(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  (void)user_data;
  (void)kind;
  (void)url;
  (void)out_response;
  return MLN_STATUS_OK;
}

static mln_status add_no_headers(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) {
  (void)user_data;
  (void)kind;
  (void)url;
  (void)out_response;
  return MLN_STATUS_OK;
}

static mln_status submit_resource_transform(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_transform(
    *(const mln_runtime*)context, descriptor, &completion.descriptor, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    (void)mln_test_completion_finish(&completion);
  } else {
    mln_test_completion_reject(&completion);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

static void resource_transform_with_zero_size(void* descriptor) {
  ((mln_resource_transform*)descriptor)->size = 0;
}

static void resource_transform_without_callback(void* descriptor) {
  ((mln_resource_transform*)descriptor)->callback = NULL;
}

static const mln_test_validation_case resource_transform_cases[] = {
  {"a well-formed transform", NULL, MLN_STATUS_OK, NULL},
  {"a zero size", resource_transform_with_zero_size,
   MLN_STATUS_INVALID_ARGUMENT, "size"},
  {"a null callback", resource_transform_without_callback,
   MLN_STATUS_INVALID_ARGUMENT, "callback"},
};

static void resource_transform_registration_validates_its_descriptor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = keep_url,
  };
  mln_test_run_validation_table(
    resource_transform_cases,
    sizeof(resource_transform_cases) / sizeof(resource_transform_cases[0]),
    &transform, sizeof(transform), submit_resource_transform, &runtime
  );
  MLN_TEST_INVALID(mln_test_set_resource_transform(runtime, NULL));
  MLN_TEST_INVALID(mln_test_clear_resource_transform(MLN_HANDLE_NULL));
  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  mln_test_destroy_runtime(runtime);
}

// Outside a transform callback there is no invocation to hold a URL, and
// inside one the helper still checks what it copies.
static void a_replacement_url_needs_a_transform_invocation(void) {
  mln_resource_transform_response response = {
    .size = sizeof(mln_resource_transform_response),
  };
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_resource_transform_response_set_url(
                                &response, "http://127.0.0.1/", 17, NULL
                              )
  );
  MLN_TEST_INVALID(mln_resource_transform_response_set_url(NULL, "x", 1, NULL));
  response.size = 0;
  MLN_TEST_INVALID(
    mln_resource_transform_response_set_url(&response, "x", 1, NULL)
  );
}

static mln_status submit_header_transform(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_http_header_transform(
    *(const mln_runtime*)context, descriptor, &completion.descriptor, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    (void)mln_test_completion_finish(&completion);
  } else {
    mln_test_completion_reject(&completion);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

static void header_transform_with_zero_size(void* descriptor) {
  ((mln_http_header_transform*)descriptor)->size = 0;
}

static void header_transform_without_callback(void* descriptor) {
  ((mln_http_header_transform*)descriptor)->callback = NULL;
}

// A transport that cannot drop a transformed header when a redirect changes
// origin reports MLN_STATUS_UNSUPPORTED rather than leak the credential to the
// redirect's destination.
static const mln_test_validation_case header_transform_cases[] = {
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  {"a well-formed transform", NULL, MLN_STATUS_OK, NULL},
#else
  {"a well-formed transform", NULL, MLN_STATUS_UNSUPPORTED, NULL},
#endif
  {"a zero size", header_transform_with_zero_size, MLN_STATUS_INVALID_ARGUMENT,
   "size"},
  {"a null callback", header_transform_without_callback,
   MLN_STATUS_INVALID_ARGUMENT, "callback"},
};

static void header_transform_registration_validates_its_descriptor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = add_no_headers,
  };
  mln_test_run_validation_table(
    header_transform_cases,
    sizeof(header_transform_cases) / sizeof(header_transform_cases[0]),
    &transform, sizeof(transform), submit_header_transform, &runtime
  );
  MLN_TEST_INVALID(mln_test_set_http_header_transform(runtime, NULL));
  MLN_TEST_INVALID(mln_test_clear_http_header_transform(MLN_HANDLE_NULL));
  mln_test_destroy_runtime(runtime);
}

typedef struct header_case {
  const char* label;
  const char* name;
  size_t name_size;
  const char* value;
  size_t value_size;
} header_case;

static const char invalid_utf8[] = {'b', 'a', 'd', (char)0xFF};

// Headers the helper refuses: malformed names, values that could split the
// request, values that are not UTF-8, and headers MapLibre or the transport
// manage themselves.
static const header_case refused_headers[] = {
  {"a name with a space", "Bad Name", 8, "value", 5},
  {"a value with a line break", "Authorization", 13, "bad\r\nvalue", 10},
  {"a managed header", "Range", 5, "bytes=0-1", 9},
  {"a value that is not UTF-8", "Authorization", 13, invalid_utf8,
   sizeof(invalid_utf8)},
  {"a null name", NULL, 1, "value", 5},
};

static void a_header_needs_a_transform_invocation_and_a_valid_field(void) {
  mln_http_header_transform_response response = {
    .size = sizeof(mln_http_header_transform_response),
  };
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_http_header_transform_response_set(
                                &response, "X-Test", 6, "value", 5, NULL
                              )
  );
  for (size_t index = 0;
       index < sizeof(refused_headers) / sizeof(refused_headers[0]);
       index += 1) {
    const header_case* row = &refused_headers[index];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_http_header_transform_response_set(
        &response, row->name, row->name_size, row->value, row->value_size, NULL
      ),
      row->label
    );
  }
}

#if !defined(__EMSCRIPTEN__)

// Rewrites URLs whose path starts /a/ to /b/, and counts its calls.
typedef struct rewriting_transform {
  atomic_int calls;
} rewriting_transform;

static mln_status rewrite_a_to_b(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  (void)kind;
  rewriting_transform* transform = user_data;
  atomic_fetch_add(&transform->calls, 1);
  const char* path = strstr(url, "/a/");
  if (path == NULL) {
    return MLN_STATUS_OK;
  }
  char rewritten[512];
  const int written = snprintf(
    rewritten, sizeof(rewritten), "%.*sb/%s", (int)(path - url + 1), url,
    path + 3
  );
  if (written <= 0 || (size_t)written >= sizeof(rewritten)) {
    return MLN_STATUS_NATIVE_ERROR;
  }
  return mln_resource_transform_response_set_url(
    out_response, rewritten, (size_t)written, NULL
  );
}

static void load_from_server(
  mln_runtime runtime, mln_test_http_server* server, const char* path,
  bool expect_loaded
) {
  char url[256];
  mln_test_http_server_url(server, path, url, sizeof(url));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  if (expect_loaded) {
    TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), path);
  } else {
    char message[512];
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_await_loading_failure(runtime, map, message, sizeof(message)),
      path
    );
  }
  mln_test_destroy_map(map);
}

// A transform's replacement URL is the one the transport fetches. Once the
// transform is cleared, requests go out as the style names them, and the
// transform is not called again.
static void a_resource_transform_rewrites_until_cleared(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/b/first.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/a/second.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 2);
  rewriting_transform probe;
  atomic_init(&probe.calls, 0);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = rewrite_a_to_b,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));

  load_from_server(runtime, server, "/a/first.json", true);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/b/first.json")
  );
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/a/first.json")
  );
  const int calls = atomic_load(&probe.calls);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(1, calls);

  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  load_from_server(runtime, server, "/a/second.json", true);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/a/second.json")
  );
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/b/second.json")
  );
  TEST_ASSERT_EQUAL_INT(calls, atomic_load(&probe.calls));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// Records the kind each transform reports for the image source's URL.
typedef struct image_kind_probe {
  atomic_uint resource_kind;
  atomic_bool resource_seen;
  atomic_uint header_kind;
  atomic_bool header_seen;
} image_kind_probe;

static const char overlay_path[] = "/overlay.png";

static mln_status record_image_resource_kind(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) {
  (void)out_response;
  image_kind_probe* probe = user_data;
  if (strstr(url, overlay_path) != NULL) {
    atomic_store(&probe->resource_kind, kind);
    mln_test_flag_set(&probe->resource_seen);
  }
  return MLN_STATUS_OK;
}

#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
static mln_status record_image_header_kind(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) {
  (void)out_response;
  image_kind_probe* probe = user_data;
  if (strstr(url, overlay_path) != NULL) {
    atomic_store(&probe->header_kind, kind);
    mln_test_flag_set(&probe->header_seen);
  }
  return MLN_STATUS_OK;
}
#endif

// An image source's URL reaches both transforms as MLN_RESOURCE_KIND_IMAGE,
// through the resource loader and through the platform transport.
static void an_image_source_request_reaches_the_transforms_as_an_image(void) {
  mln_test_http_server* server = mln_test_http_server_start(NULL, 0);
  image_kind_probe probe;
  atomic_init(&probe.resource_kind, MLN_RESOURCE_KIND_UNKNOWN);
  atomic_init(&probe.resource_seen, false);
  atomic_init(&probe.header_kind, MLN_RESOURCE_KIND_UNKNOWN);
  atomic_init(&probe.header_seen, false);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = record_image_resource_kind,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  const mln_http_header_transform header_transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = record_image_header_kind,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &header_transform));
#endif

  char overlay_url[256];
  mln_test_http_server_url(
    server, overlay_path, overlay_url, sizeof(overlay_url)
  );
  char style[512];
  const int written = snprintf(
    style, sizeof(style),
    "{\"version\":8,\"sources\":{\"overlay\":{\"type\":\"image\",\"url\":"
    "\"%s\",\"coordinates\":[[-1,1],[1,1],[1,-1],[-1,-1]]}},\"layers\":[]}",
    overlay_url
  );
  TEST_ASSERT_TRUE(written > 0 && (size_t)written < sizeof(style));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(
    map, (mln_buffer_view){.data = style, .size = (size_t)written}
  ));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&probe.resource_seen),
    "the resource transform never saw the image request"
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_KIND_IMAGE, atomic_load(&probe.resource_kind)
  );
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&probe.header_seen),
    "the header transform never saw the image request"
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_KIND_IMAGE, atomic_load(&probe.header_kind)
  );
#endif
  TEST_ASSERT_TRUE(
    mln_test_http_server_wait_for_requests(server, overlay_path, 1)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

#endif

#if !defined(__EMSCRIPTEN__) && MLN_TEST_HEADER_TRANSFORM_SUPPORTED

static const char token_header[] = "X-Test-Token";
static const char token_value[] = "secret-token";

// Adds the token header to every request, and records the kinds and the last
// URL it saw.
typedef struct token_transform {
  atomic_int calls;
  atomic_uint last_kind;
} token_transform;

static mln_status add_token(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) {
  (void)url;
  token_transform* transform = user_data;
  atomic_fetch_add(&transform->calls, 1);
  atomic_store(&transform->last_kind, kind);
  return mln_http_header_transform_response_set(
    out_response, token_header, sizeof(token_header) - 1, token_value,
    sizeof(token_value) - 1, NULL
  );
}

static void install_token_transform(
  mln_runtime runtime, token_transform* probe
) {
  atomic_init(&probe->calls, 0);
  atomic_init(&probe->last_kind, 0);
  const mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = add_token,
    .user_data = probe,
  };
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &transform));
}

static bool request_carried_token(
  mln_test_http_server* server, const char* path
) {
  char value[128];
  return mln_test_http_server_request_header(
           server, path, 0, token_header, value, sizeof(value)
         ) &&
         strcmp(value, token_value) == 0;
}

// The transform's headers go out on HTTP requests until it is cleared.
static void a_header_transform_adds_headers_until_cleared(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/with-token.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/after-clear.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 2);
  mln_runtime runtime = mln_test_create_runtime();
  token_transform probe;
  install_token_transform(runtime, &probe);

  load_from_server(runtime, server, "/with-token.json", true);
  TEST_ASSERT_TRUE(request_carried_token(server, "/with-token.json"));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_KIND_STYLE, atomic_load(&probe.last_kind)
  );
  const int calls = atomic_load(&probe.calls);

  MLN_TEST_OK(mln_test_clear_http_header_transform(runtime));
  load_from_server(runtime, server, "/after-clear.json", true);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/after-clear.json")
  );
  TEST_ASSERT_FALSE(request_carried_token(server, "/after-clear.json"));
  TEST_ASSERT_EQUAL_INT(calls, atomic_load(&probe.calls));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// A file:// style never reaches the HTTP client, so the transform is not
// called for it.
static void a_header_transform_skips_requests_that_are_not_http(void) {
  char path[1024];
  mln_test_temp_path("header-transform-style.json", path, sizeof(path));
  FILE* file = fopen(path, "wb");
  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_size_t(
    sizeof(empty_style_json) - 1,
    fwrite(empty_style_json, 1, sizeof(empty_style_json) - 1, file)
  );
  TEST_ASSERT_EQUAL_INT(0, fclose(file));
  char url[1100];
  (void)snprintf(
    url, sizeof(url), "file://%s%s", path[0] == '/' ? "" : "/", path
  );
  for (char* cursor = url; *cursor != '\0'; cursor += 1) {
    if (*cursor == '\\') *cursor = '/';
  }

  mln_runtime runtime = mln_test_create_runtime();
  token_transform probe;
  install_token_transform(runtime, &probe);
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  (void)remove(path);
}

// A redirect to the same origin keeps the transform's headers, and one to
// another origin drops them before the request leaves.
static void a_redirect_keeps_headers_only_within_the_origin(void) {
  static const mln_test_http_route other_routes[] = {
    {.path = "/elsewhere.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* other = mln_test_http_server_start(other_routes, 1);
  static char cross_origin_location[256];
  char elsewhere[200];
  mln_test_http_server_url(
    other, "/elsewhere.json", elsewhere, sizeof(elsewhere)
  );
  (void)snprintf(
    cross_origin_location, sizeof(cross_origin_location), "Location: %s\r\n",
    elsewhere
  );
  const mln_test_http_route routes[] = {
    {.path = "/same-origin.json",
     .status = 302,
     .headers = "Location: /redirected.json\r\n"},
    {.path = "/redirected.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/cross-origin.json",
     .status = 302,
     .headers = cross_origin_location},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 3);
  mln_runtime runtime = mln_test_create_runtime();
  token_transform probe;
  install_token_transform(runtime, &probe);

  load_from_server(runtime, server, "/same-origin.json", true);
  TEST_ASSERT_TRUE(request_carried_token(server, "/same-origin.json"));
  TEST_ASSERT_TRUE(request_carried_token(server, "/redirected.json"));

  load_from_server(runtime, server, "/cross-origin.json", true);
  TEST_ASSERT_TRUE(request_carried_token(server, "/cross-origin.json"));
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(other, "/elsewhere.json")
  );
  TEST_ASSERT_FALSE(request_carried_token(other, "/elsewhere.json"));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
  mln_test_http_server_stop(other);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(resource_transform_registration_validates_its_descriptor);
  RUN_TEST(a_replacement_url_needs_a_transform_invocation);
  RUN_TEST(header_transform_registration_validates_its_descriptor);
  RUN_TEST(a_header_needs_a_transform_invocation_and_a_valid_field);
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_resource_transform_rewrites_until_cleared);
  RUN_TEST(an_image_source_request_reaches_the_transforms_as_an_image);
#endif
#if !defined(__EMSCRIPTEN__) && MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  RUN_TEST(a_header_transform_adds_headers_until_cleared);
  RUN_TEST(a_header_transform_skips_requests_that_are_not_http);
  RUN_TEST(a_redirect_keeps_headers_only_within_the_origin);
#endif
}
