#ifndef MLN_NATIVE_TESTS_RESOURCES_H
#define MLN_NATIVE_TESTS_RESOURCES_H

// Helpers for the resource suites: settling runtime resource configuration
// commands, and a scripted resource provider that answers from a table and
// records every request it sees.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Settles a runtime configuration command: a rejected submission leaves the
// completion with the caller, and an accepted one reports its terminal status.
mln_status mln_test_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider
);
mln_status mln_test_clear_resource_provider(mln_runtime runtime);
mln_status mln_test_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform
);
mln_status mln_test_clear_resource_transform(mln_runtime runtime);
mln_status mln_test_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform
);
mln_status mln_test_clear_http_header_transform(mln_runtime runtime);

// Waits for the map's MAP_LOADING_FAILED event and copies its message.
bool mln_test_await_loading_failure(
  mln_runtime runtime, mln_map map, char* out_message, size_t capacity
);
// Waits for the map's MAP_STYLE_LOADED event. Reports false, with the failure
// message as the case's message, when the map fails to load instead.
bool mln_test_await_style_loaded(mln_runtime runtime, mln_map map);

// One resource the scripted provider answers. `response` is the answer; its
// size field is filled in for the case. A held resource is claimed and left
// unanswered for the case to complete through its recorded handle. A case that
// answers a repeat request differently, such as a revalidation, scripts that
// answer as `later_response` and `later_held`, which apply to every request for
// the URL after the first; the table stays unchanged while the provider reads
// it from file source threads.
typedef struct mln_test_provided_resource {
  const char* url;
  mln_resource_response response;
  bool held;
  bool has_later_response;
  mln_resource_response later_response;
  bool later_held;
} mln_test_provided_resource;

// What the provider saw of one request, copied out of the callback.
typedef struct mln_test_provider_request {
  atomic_bool recorded;
  char requested_url[256];
  char resolved_url[256];
  uint32_t kind;
  uint32_t usage;
  bool has_range;
  uint64_t range_start;
  uint64_t range_end;
  bool has_prior_modified;
  int64_t prior_modified_unix_ms;
  bool has_prior_expires;
  int64_t prior_expires_unix_ms;
  bool has_prior_etag;
  char prior_etag[64];
  size_t prior_data_size;
  // Set for a held resource; the case completes and releases it.
  mln_resource_request_handle handle;
} mln_test_provider_request;

#define MLN_TEST_PROVIDER_REQUEST_CAPACITY 64

// Answers the listed resources and fails every other request as not found, so
// a case never reaches the network. It records each request, pulses, and only
// then answers, so a case that observes the answer also observes the record.
// Heap-allocate it or give it static storage: a failing case leaves its
// runtime for the harness to reclaim, and the provider can run until then.
typedef struct mln_test_provider {
  const mln_test_provided_resource* resources;
  size_t resource_count;
  // How many requests each listed resource has answered or held.
  atomic_int* matches;
  atomic_int request_count;
  mln_test_provider_request requests[MLN_TEST_PROVIDER_REQUEST_CAPACITY];
} mln_test_provider;

// Allocates a provider for `resources`, which must outlive it. Free it with
// mln_test_provider_destroy() once the runtime that used it is released.
mln_test_provider* mln_test_provider_create(
  const mln_test_provided_resource* resources, size_t resource_count
);
void mln_test_provider_destroy(mln_test_provider* provider);
// Registers the provider on `runtime` and fails the case when it is refused.
void mln_test_provider_install(
  mln_runtime runtime, mln_test_provider* provider
);
// How many requests for `url` the provider has recorded.
int mln_test_provider_requests(mln_test_provider* provider, const char* url);
// Waits within the default deadline for `count` requests for `url`.
bool mln_test_provider_wait_for_requests(
  mln_test_provider* provider, const char* url, int count
);
// The `index`th recorded request for `url`, or null.
const mln_test_provider_request* mln_test_provider_request_at(
  mln_test_provider* provider, const char* url, int index
);

// A response carrying `text` as its bytes, with every other field unset.
mln_resource_response mln_test_text_response(const char* text);
// A provider error with `reason` and `message`.
mln_resource_response mln_test_error_response(
  uint32_t reason, const char* message
);

#ifdef __cplusplus
}
#endif

#endif
