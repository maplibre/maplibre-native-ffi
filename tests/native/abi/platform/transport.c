// The platform's resource transports: local files and PMTiles archives on
// every target, and, against a loopback server, the HTTP client's handling of
// statuses, redirects, revalidation, compression, byte ranges, and a request
// that is still in flight when its map goes away. The browser's HTTP client
// runs against the runner's routes in browser_http_emscripten.c instead.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "support/harness.h"
#include "support/map.h"
#include "support/resources.h"
#include "support/test_support.h"
#include "unity.h"

#if !defined(__EMSCRIPTEN__)
#include "support/http_server.h"
#endif

static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";
static const char pmtiles_fixture[] =
  "storage/pmtiles/geography-class-png.pmtiles";

// Writes `url` as a file URL for `path`, percent-encoding every byte outside
// the unreserved set and the path separators.
static void file_url(const char* path, char* url, size_t capacity) {
  static const char hex[] = "0123456789ABCDEF";
  size_t used = (size_t)snprintf(url, capacity, "file://");
  if (path[0] != '/') {
    url[used++] = '/';
  }
  for (const unsigned char* cursor = (const unsigned char*)path;
       *cursor != '\0'; cursor += 1) {
    TEST_ASSERT_TRUE_MESSAGE(used + 4 < capacity, "the file URL does not fit");
    const unsigned char byte = *cursor == '\\' ? '/' : *cursor;
    const bool plain =
      (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
      (byte >= '0' && byte <= '9') || byte == '-' || byte == '.' ||
      byte == '_' || byte == '~' || byte == '/' || byte == ':';
    if (plain) {
      url[used++] = (char)byte;
    } else {
      url[used++] = '%';
      url[used++] = hex[byte >> 4];
      url[used++] = hex[byte & 0x0F];
    }
  }
  url[used] = '\0';
}

// Opens `path`, which may hold UTF-8 beyond ASCII, for writing.
static FILE* open_for_writing(const char* path) {
#if defined(_WIN32)
  wchar_t wide[1024];
  if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, 1024) == 0) {
    return NULL;
  }
  return _wfopen(wide, L"wb");
#else
  return fopen(path, "wb");
#endif
}

static void remove_path(const char* path) {
#if defined(_WIN32)
  wchar_t wide[1024];
  if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, 1024) != 0) {
    (void)_wremove(wide);
  }
#else
  (void)remove(path);
#endif
}

// A file:// URL names its path percent-encoded, and the local file source
// decodes it, including bytes beyond ASCII.
static void a_percent_encoded_file_url_loads_its_file(void) {
  char path[1024];
  mln_test_temp_path("caf\xC3\xA9 style #1.json", path, sizeof(path));
  FILE* file = open_for_writing(path);
  TEST_ASSERT_NOT_NULL_MESSAGE(file, path);
  TEST_ASSERT_EQUAL_size_t(
    sizeof(empty_style_json) - 1,
    fwrite(empty_style_json, 1, sizeof(empty_style_json) - 1, file)
  );
  TEST_ASSERT_EQUAL_INT(0, fclose(file));
  char url[4096];
  file_url(path, url, sizeof(url));
  TEST_ASSERT_NOT_NULL(strstr(url, "caf%C3%A9%20style%20%231.json"));

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  remove_path(path);
}

static mln_map create_static_map(mln_runtime runtime) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  return mln_test_create_map_with_options(runtime, &options);
}

// Renders a raster source whose archive `archive_url` names, and reports the
// still image's status.
static mln_status render_pmtiles(mln_runtime runtime, const char* archive_url) {
  char style[2048];
  const int written = snprintf(
    style, sizeof(style),
    "{\"version\":8,\"sources\":{\"archive\":{\"type\":\"raster\","
    "\"url\":\"pmtiles://%s\",\"tileSize\":256}},\"layers\":[{\"id\":"
    "\"raster\",\"type\":\"raster\",\"source\":\"archive\"}]}",
    archive_url
  );
  TEST_ASSERT_TRUE(written > 0 && (size_t)written < sizeof(style));
  mln_map map = create_static_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_buffer_view(style, (size_t)written)
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const mln_status status = mln_test_render_still_image(&fixture, map);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  return status;
}

// A PMTiles archive on disk is read in ranges: its header, directory, and the
// tile the map renders.
static void a_local_pmtiles_archive_renders(void) {
  char path[1024];
  TEST_ASSERT_TRUE(mln_test_fixture_path(pmtiles_fixture, path, sizeof(path)));
  char url[4096];
  file_url(path, url, sizeof(url));
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(render_pmtiles(runtime, url));
  mln_test_destroy_runtime(runtime);
}

#if !defined(__EMSCRIPTEN__)

typedef enum style_outcome {
  STYLE_LOADS,
  STYLE_FAILS,
} style_outcome;

typedef struct transport_case {
  const char* label;
  const char* path;
  style_outcome outcome;
  // A fragment the failure message must contain.
  const char* failure;
  // A path the server must also have been asked for, such as a redirect's
  // target, or null.
  const char* also_requested;
} transport_case;

static const uint8_t gzipped_style[] = {
  0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xab, 0x56, 0x2a,
  0x4b, 0x2d, 0x2a, 0xce, 0xcc, 0xcf, 0x53, 0xb2, 0xb2, 0xd0, 0x51, 0xca, 0x4b,
  0xcc, 0x4d, 0x55, 0xb2, 0x52, 0x4a, 0xaf, 0xca, 0x2c, 0x28, 0x48, 0x4d, 0x51,
  0xd2, 0x51, 0x2a, 0xce, 0x2f, 0x2d, 0x4a, 0x4e, 0x2d, 0x56, 0xb2, 0xaa, 0xae,
  0xd5, 0x51, 0xca, 0x49, 0xac, 0x04, 0xaa, 0x55, 0xb2, 0x8a, 0x8e, 0xad, 0x05,
  0x00, 0x19, 0x4b, 0xec, 0x30, 0x37, 0x00, 0x00, 0x00,
};

static const mln_test_http_route transport_routes[] = {
  {.path = "/ok.json",
   .body = empty_style_json,
   .body_size = sizeof(empty_style_json) - 1},
  {.path = "/server-error.json", .status = 500},
  {.path = "/unavailable.json", .status = 503},
  {.path = "/moved.json", .status = 301, .headers = "Location: /ok.json\r\n"},
  {.path = "/found.json", .status = 302, .headers = "Location: ./ok.json\r\n"},
  {.path = "/temporary.json",
   .status = 307,
   .headers = "Location: /ok.json\r\n"},
  {.path = "/to-missing.json",
   .status = 302,
   .headers = "Location: /missing.json\r\n"},
  {.path = "/gzipped.json",
   .headers = "Content-Type: application/json\r\nContent-Encoding: gzip\r\n",
   .body = gzipped_style,
   .body_size = sizeof(gzipped_style)},
};

static const transport_case transport_cases[] = {
  {"200", "/ok.json", STYLE_LOADS, NULL, NULL},
  {"404", "/missing.json", STYLE_FAILS, "404", NULL},
  {"500", "/server-error.json", STYLE_FAILS, "500", NULL},
  {"503", "/unavailable.json", STYLE_FAILS, "503", NULL},
  {"301", "/moved.json", STYLE_LOADS, NULL, "/ok.json"},
  {"302 to a relative path", "/found.json", STYLE_LOADS, NULL, "/ok.json"},
  {"307", "/temporary.json", STYLE_LOADS, NULL, "/ok.json"},
  {"a redirect to a missing resource", "/to-missing.json", STYLE_FAILS, "404",
   "/missing.json"},
  {"a gzipped body", "/gzipped.json", STYLE_LOADS, NULL, NULL},
};

// Each HTTP answer reaches the map as the status says: a style that loads,
// through any redirects, or a loading failure that names the status.
static void the_http_transport_follows_each_status(void) {
  mln_test_http_server* server = mln_test_http_server_start(
    transport_routes, sizeof(transport_routes) / sizeof(transport_routes[0])
  );
  mln_runtime runtime = mln_test_create_runtime();
  for (size_t index = 0;
       index < sizeof(transport_cases) / sizeof(transport_cases[0]);
       index += 1) {
    const transport_case* row = &transport_cases[index];
    char url[256];
    mln_test_http_server_url(server, row->path, url, sizeof(url));
    mln_map map = mln_test_create_map(runtime);
    MLN_TEST_OK(mln_test_map_set_style_url(map, url));
    if (row->outcome == STYLE_LOADS) {
      TEST_ASSERT_TRUE_MESSAGE(
        mln_test_await_style_loaded(runtime, map), row->label
      );
    } else {
      char message[512];
      TEST_ASSERT_TRUE_MESSAGE(
        mln_test_await_loading_failure(runtime, map, message, sizeof(message)),
        row->label
      );
      TEST_ASSERT_NOT_NULL_MESSAGE(strstr(message, row->failure), message);
    }
    if (row->also_requested != NULL) {
      TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(
        1, mln_test_http_server_requests(server, row->also_requested),
        row->label
      );
    }
    mln_test_destroy_map(map);
  }
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// A port that nothing listens on refuses the connection, which reaches the map
// as a loading failure rather than a hang. The port is one a server just gave
// up, so nothing else on the host is listening there.
static void a_refused_connection_fails_the_load(void) {
  mln_test_http_server* server = mln_test_http_server_start(NULL, 0);
  char url[256];
  mln_test_http_server_url(server, "/refused.json", url, sizeof(url));
  mln_test_http_server_stop(server);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A cached response that must be revalidated goes out again with its ETag, and
// a 304 answer delivers the cached bytes.
static void a_not_modified_response_revalidates_the_cached_style(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/revalidated.json",
     .headers = "Cache-Control: max-age=0, must-revalidate\r\n",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1,
     .etag = "\"e1\""},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char url[256];
  mln_test_http_server_url(server, "/revalidated.json", url, sizeof(url));
  mln_runtime runtime = mln_test_create_runtime();
  // The default cache is in memory and lives only while something holds it,
  // so the first map stays until the second has loaded.
  mln_map first = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(first, url));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, first));
  char validator[64];
  TEST_ASSERT_FALSE(mln_test_http_server_request_header(
    server, "/revalidated.json", 0, "If-None-Match", validator,
    sizeof(validator)
  ));

  mln_map second = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(second, url));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, second));
  TEST_ASSERT_TRUE(mln_test_http_server_request_header(
    server, "/revalidated.json", 1, "If-None-Match", validator,
    sizeof(validator)
  ));
  TEST_ASSERT_EQUAL_STRING("\"e1\"", validator);
  mln_test_destroy_map(second);
  mln_test_destroy_map(first);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// A PMTiles archive over HTTP is read with Range requests, which the server
// answers with the slices the archive's directory names.
static void a_remote_pmtiles_archive_is_read_in_ranges(void) {
  size_t size = 0;
  uint8_t* archive = mln_test_read_fixture(pmtiles_fixture, &size);
  TEST_ASSERT_NOT_NULL(archive);
  const mln_test_http_route routes[] = {
    {.path = "/archive.pmtiles",
     .body = archive,
     .body_size = size,
     .serves_ranges = true},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char url[256];
  mln_test_http_server_url(server, "/archive.pmtiles", url, sizeof(url));
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(render_pmtiles(runtime, url));

  const int requests =
    mln_test_http_server_requests(server, "/archive.pmtiles");
  TEST_ASSERT_GREATER_OR_EQUAL_INT(2, requests);
  for (int index = 0; index < requests; index += 1) {
    char range[64];
    TEST_ASSERT_TRUE(mln_test_http_server_request_header(
      server, "/archive.pmtiles", index, "Range", range, sizeof(range)
    ));
    TEST_ASSERT_EQUAL_STRING_LEN("bytes=", range, 6);
  }
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
  free(archive);
}

// Releasing a map whose style request the server is still holding completes
// without waiting for the response. The server answers only afterwards, and
// the runtime then closes with nothing left open. Nothing fences the transport
// thread against the late answer, so the case asserts no event for it.
static void a_request_in_flight_does_not_hold_its_map_open(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/held.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1,
     .held = true},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char url[256];
  mln_test_http_server_url(server, "/held.json", url, sizeof(url));
  mln_runtime runtime = mln_test_create_runtime();
  // The case waits for the map's release itself, so it creates it untracked.
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &map));
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE(
    mln_test_http_server_wait_for_requests(server, "/held.json", 1)
  );

  MLN_TEST_OK(mln_test_map_close(map));
  mln_test_http_server_release_held(server);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

#if defined(__APPLE__)
// NSURLSession gives up on a redirect loop with an error the Apple transport
// has no specific reason for, and the map still receives it as a loading
// failure that carries the system's description.
static void an_unclassified_transport_error_fails_the_style(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/loop.json",
     .status = 302,
     .headers = "Location: /loop.json\r\n"},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char url[256];
  mln_test_http_server_url(server, "/loop.json", url, sizeof(url));
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  char message[512];
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(message, "redirect"), message);
  TEST_ASSERT_GREATER_THAN_INT(
    1, mln_test_http_server_requests(server, "/loop.json")
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}
#endif

#endif

MLN_TEST_GROUP {
  RUN_TEST(a_percent_encoded_file_url_loads_its_file);
  RUN_TEST(a_local_pmtiles_archive_renders);
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(the_http_transport_follows_each_status);
  RUN_TEST(a_refused_connection_fails_the_load);
  RUN_TEST(a_not_modified_response_revalidates_the_cached_style);
  RUN_TEST(a_remote_pmtiles_archive_is_read_in_ranges);
  RUN_TEST(a_request_in_flight_does_not_hold_its_map_open);
#if defined(__APPLE__)
  RUN_TEST(an_unclassified_transport_error_fails_the_style);
#endif
#endif
}
