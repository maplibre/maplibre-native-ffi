// What the exit programs share: a latch for one completion, and the runtime,
// map, and style that each program leaves live as it exits.
//
// A program sets up native work that keeps native threads busy, then returns
// from main without releasing, clearing, or waiting for anything, so that the
// process exits while the library's threads are still at work. Every callback
// it installs stays installed through the exit.

#ifndef MLN_NATIVE_EXIT_PROBE_H
#define MLN_NATIVE_EXIT_PROBE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maplibre_native_c.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// Waits for one completion, which a native thread delivers. The deliverer
// writes the result before it signals, and the waiter reads it after.
typedef struct probe_latch {
#ifdef _WIN32
  HANDLE delivered;
#else
  pthread_mutex_t lock;
  pthread_cond_t changed;
  bool done;
#endif
  mln_status status;
  // The completion's value, for a creation that delivers a handle.
  uint64_t handle;
} probe_latch;

static inline void probe_latch_init(probe_latch* latch) {
  memset(latch, 0, sizeof(*latch));
#ifdef _WIN32
  latch->delivered = CreateEventW(NULL, TRUE, FALSE, NULL);
#else
  pthread_mutex_init(&latch->lock, NULL);
  pthread_cond_init(&latch->changed, NULL);
#endif
}

static inline void probe_latch_deliver(
  void* user_data, const mln_completion_result* result
) {
  probe_latch* latch = user_data;
  uint64_t handle = 0;
  if (result->value != NULL && result->value_count == 1) {
    memcpy(&handle, result->value, sizeof(handle));
  }
#ifdef _WIN32
  latch->status = result->status;
  latch->handle = handle;
  SetEvent(latch->delivered);
#else
  pthread_mutex_lock(&latch->lock);
  latch->status = result->status;
  latch->handle = handle;
  latch->done = true;
  pthread_cond_broadcast(&latch->changed);
  pthread_mutex_unlock(&latch->lock);
#endif
}

static inline mln_completion probe_latch_completion(probe_latch* latch) {
  probe_latch_init(latch);
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = probe_latch_deliver,
    .user_data = latch,
  };
}

// The program's CTest entry bounds the wait.
static inline mln_status probe_latch_wait(probe_latch* latch) {
#ifdef _WIN32
  WaitForSingleObject(latch->delivered, INFINITE);
  CloseHandle(latch->delivered);
#else
  pthread_mutex_lock(&latch->lock);
  while (!latch->done) {
    pthread_cond_wait(&latch->changed, &latch->lock);
  }
  pthread_mutex_unlock(&latch->lock);
#endif
  return latch->status;
}

static inline void probe_require(mln_status status, const char* what) {
  if (status != MLN_STATUS_OK) {
    (void)fprintf(stderr, "%s failed with status %d\n", what, (int)status);
    exit(1);
  }
}

// Submits a command that takes `completion` and waits for its terminal status.
#define PROBE_AWAIT(what, expression)                           \
  do {                                                          \
    probe_latch latch;                                          \
    mln_completion completion = probe_latch_completion(&latch); \
    probe_require((expression), what);                          \
    probe_require(probe_latch_wait(&latch), what);              \
  } while (false)

static inline void probe_discard(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  (void)result;
}

static const mln_completion probe_discarding_completion = {
  .size = sizeof(mln_completion),
  .callback = probe_discard,
};

static inline uint32_t probe_consume_log(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) {
  (void)user_data;
  (void)severity;
  (void)event;
  (void)code;
  (void)message;
  return 1;
}

static inline void probe_ignore_wake(void* user_data) { (void)user_data; }

// A style whose GeoJSON source keeps the tile workers busy. Its glyph and
// sprite requests reach the resource provider, which fails them, and the
// failures reach the log callback.
static const char probe_style_json[] =
  "{\"version\":8,"
  "\"glyphs\":\"custom://glyphs/{fontstack}/{range}.pbf\","
  "\"sprite\":\"custom://sprite\","
  "\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "\"custom://points.geojson\"}},"
  "\"layers\":["
  "{\"id\":\"background\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#204060\"}},"
  "{\"id\":\"circles\",\"type\":\"circle\",\"source\":\"points\","
  "\"paint\":{\"circle-radius\":[\"+\",2,[\"%\",[\"get\",\"n\"],5]]}},"
  "{\"id\":\"labels\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"text-field\":[\"to-string\",[\"get\",\"n\"]],"
  "\"icon-image\":\"marker\"}}]}";

// The GeoJSON data that the style's source loads, which main writes before it
// creates a runtime: 500 points spread over the world.
static char probe_points_json[64 * 1024];

static inline void probe_write_points(void) {
  size_t length = (size_t)snprintf(
    probe_points_json, sizeof(probe_points_json),
    "{\"type\":\"FeatureCollection\",\"features\":["
  );
  for (int index = 0; index < 500; index += 1) {
    length += (size_t)snprintf(
      probe_points_json + length, sizeof(probe_points_json) - length,
      "%s{\"type\":\"Feature\",\"properties\":{\"n\":%d},"
      "\"geometry\":{\"type\":\"Point\",\"coordinates\":[%d,%d]}}",
      index == 0 ? "" : ",", index, (index * 37) % 340 - 170,
      (index * 53) % 160 - 80
    );
  }
  (void)snprintf(
    probe_points_json + length, sizeof(probe_points_json) - length, "]}"
  );
}

// Serves the style and its GeoJSON data, and fails every other request, from
// inside the callback on the file source thread that asks.
static inline uint32_t probe_provide(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)user_data;
  const char* url =
    request->requested_url != NULL ? request->requested_url : "";
  const char* body = NULL;
  if (strcmp(url, "custom://style.json") == 0) {
    body = probe_style_json;
  } else if (strcmp(url, "custom://points.geojson") == 0) {
    body = probe_points_json;
  }
  mln_resource_response response = {.size = sizeof(mln_resource_response)};
  if (body != NULL) {
    response.status = MLN_RESOURCE_RESPONSE_STATUS_OK;
    response.bytes = (const uint8_t*)body;
    response.byte_count = strlen(body);
  } else {
    response.status = MLN_RESOURCE_RESPONSE_STATUS_ERROR;
    response.error_reason = MLN_RESOURCE_ERROR_REASON_NOT_FOUND;
  }
  (void)mln_resource_request_complete(handle, &response, NULL);
  mln_resource_request_release(handle);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

// Writes the probe's data and installs the process-global log callback,
// delivering every severity on MapLibre's logging thread.
static inline void probe_start(void) {
  probe_write_points();
  const mln_log_handler handler = {
    .size = sizeof(mln_log_handler),
    .callback = probe_consume_log,
  };
  probe_require(
    mln_log_set_callback(&handler, NULL), "installing the log callback"
  );
  probe_require(
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_ALL, NULL),
    "making every log severity asynchronous"
  );
}

// Creates a runtime with an event wake and the probe's resource provider.
static inline mln_runtime probe_create_runtime(void) {
  mln_runtime_options options = mln_runtime_options_default();
  options.event_wake = (mln_wake){.callback = probe_ignore_wake};
  mln_runtime runtime = MLN_HANDLE_NULL;
  probe_require(
    mln_runtime_create(&options, &runtime, NULL), "creating a runtime"
  );
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider), .callback = probe_provide
  };
  PROBE_AWAIT(
    "installing the resource provider",
    mln_runtime_set_resource_provider(runtime, &provider, &completion, NULL)
  );
  return runtime;
}

// Creates a map on `runtime` and starts loading the probe's style, without
// waiting for the load.
static inline mln_map probe_create_map(mln_runtime runtime) {
  probe_latch latch;
  const mln_completion completion = probe_latch_completion(&latch);
  const mln_map_options options = mln_map_options_default();
  probe_require(
    mln_runtime_create_map(runtime, &options, &completion, NULL),
    "creating a map"
  );
  probe_require(probe_latch_wait(&latch), "creating a map");
  const mln_map map = latch.handle;
  probe_require(
    mln_map_set_style_url(
      map, "custom://style.json", &probe_discarding_completion, NULL
    ),
    "setting the style"
  );
  return map;
}

#endif
