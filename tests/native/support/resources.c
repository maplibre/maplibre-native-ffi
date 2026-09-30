// Resource configuration commands and the scripted resource provider.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resources.h"

#include "env.h"
#include "unity.h"
#include "wait.h"

static mln_status commit(mln_status status, mln_test_completion* completion) {
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  const mln_status result =
    status == MLN_STATUS_OK ? mln_test_completion_finish(completion) : status;
  mln_test_completion_destroy(completion);
  return result;
}

mln_status mln_test_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_resource_provider(
      runtime, provider, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    ),
    &completion
  );
}

mln_status mln_test_clear_resource_provider(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_resource_provider(
      runtime, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    ),
    &completion
  );
}

mln_status mln_test_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_resource_transform(
      runtime, transform, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    ),
    &completion
  );
}

mln_status mln_test_clear_resource_transform(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_resource_transform(
      runtime, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    ),
    &completion
  );
}

mln_status mln_test_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_set_http_header_transform(
      runtime, transform, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    ),
    &completion
  );
}

mln_status mln_test_clear_http_header_transform(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  return commit(
    mln_runtime_clear_http_header_transform(
      runtime, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    ),
    &completion
  );
}

bool mln_test_await_loading_failure(
  mln_runtime runtime, mln_map map, char* out_message, size_t capacity
) {
  mln_runtime_event event = {0};
  return mln_test_await_event(
           runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, &event,
           out_message, capacity
         ) &&
         event.message_size < capacity;
}

typedef struct style_outcome {
  mln_map map;
  bool loaded;
  char message[512];
} style_outcome;

static bool style_settled(
  const mln_runtime_event* event, const char* messages, void* context
) {
  style_outcome* outcome = context;
  if (event->source != outcome->map) {
    return false;
  }
  if (event->type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
    outcome->loaded = true;
    return true;
  }
  if (event->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
    const size_t size = event->message_size < sizeof(outcome->message)
                          ? event->message_size
                          : sizeof(outcome->message) - 1;
    memcpy(outcome->message, messages + event->message_offset, size);
    outcome->message[size] = '\0';
    return true;
  }
  return false;
}

bool mln_test_await_style_loaded(mln_runtime runtime, mln_map map) {
  style_outcome outcome = {.map = map, .message = "the style never settled"};
  const bool settled = mln_test_await_event_matching(
    runtime, style_settled, &outcome, mln_test_deadline_default()
  );
  if (!settled || !outcome.loaded) {
    TEST_MESSAGE(outcome.message);
  }
  return settled && outcome.loaded;
}

mln_resource_response mln_test_text_response(const char* text) {
  return (mln_resource_response){
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = (const uint8_t*)text,
    .byte_count = text == NULL ? 0 : strlen(text),
  };
}

mln_resource_response mln_test_error_response(
  uint32_t reason, const char* message
) {
  return (mln_resource_response){
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    .error_reason = reason,
    .error_message = message,
  };
}

static void copy_text(const char* text, char* out, size_t capacity) {
  (void)snprintf(out, capacity, "%s", text == NULL ? "" : text);
}

static uint32_t scripted_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  mln_test_provider* provider = user_data;
  const mln_test_provided_resource* match = NULL;
  for (size_t index = 0; index < provider->resource_count; index += 1) {
    if (
      request->requested_url != NULL &&
      strcmp(provider->resources[index].url, request->requested_url) == 0
    ) {
      match = &provider->resources[index];
      break;
    }
  }

  const int slot = atomic_fetch_add(&provider->request_count, 1);
  if (slot < MLN_TEST_PROVIDER_REQUEST_CAPACITY) {
    mln_test_provider_request* record = &provider->requests[slot];
    copy_text(
      request->requested_url, record->requested_url,
      sizeof(record->requested_url)
    );
    copy_text(
      request->resolved_url, record->resolved_url, sizeof(record->resolved_url)
    );
    record->kind = request->kind;
    record->usage = request->usage;
    record->has_range = request->has_range;
    record->range_start = request->range_start;
    record->range_end = request->range_end;
    record->has_prior_modified = request->has_prior_modified;
    record->prior_modified_unix_ms = request->prior_modified_unix_ms;
    record->has_prior_expires = request->has_prior_expires;
    record->prior_expires_unix_ms = request->prior_expires_unix_ms;
    record->has_prior_etag = request->prior_etag != NULL;
    copy_text(
      request->prior_etag, record->prior_etag, sizeof(record->prior_etag)
    );
    record->prior_data_size = request->prior_data_size;
    record->handle = match != NULL && match->held ? handle : MLN_HANDLE_NULL;
    atomic_store(&record->recorded, true);
  }
  mln_test_pulse();

  if (match != NULL && match->held) {
    return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
  }
  mln_resource_response response =
    match != NULL ? match->response
                  : mln_test_error_response(
                      MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
                      "the test provider has no route for this URL"
                    );
  response.size = sizeof(response);
  (void)mln_resource_request_complete(handle, &response, NULL);
  mln_resource_request_release(handle);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

mln_test_provider* mln_test_provider_create(
  const mln_test_provided_resource* resources, size_t resource_count
) {
  mln_test_provider* provider = calloc(1, sizeof(*provider));
  TEST_ASSERT_NOT_NULL(provider);
  provider->resources = resources;
  provider->resource_count = resource_count;
  atomic_init(&provider->request_count, 0);
  for (size_t index = 0; index < MLN_TEST_PROVIDER_REQUEST_CAPACITY;
       index += 1) {
    atomic_init(&provider->requests[index].recorded, false);
  }
  return provider;
}

void mln_test_provider_destroy(mln_test_provider* provider) { free(provider); }

void mln_test_provider_install(
  mln_runtime runtime, mln_test_provider* provider
) {
  const mln_resource_provider descriptor = {
    .size = sizeof(mln_resource_provider),
    .callback = scripted_provider,
    .user_data = provider,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_set_resource_provider(runtime, &descriptor)
  );
}

const mln_test_provider_request* mln_test_provider_request_at(
  mln_test_provider* provider, const char* url, int index
) {
  int count = atomic_load(&provider->request_count);
  if (count > MLN_TEST_PROVIDER_REQUEST_CAPACITY) {
    count = MLN_TEST_PROVIDER_REQUEST_CAPACITY;
  }
  int seen = 0;
  for (int slot = 0; slot < count; slot += 1) {
    const mln_test_provider_request* record = &provider->requests[slot];
    if (!atomic_load(&record->recorded)) {
      continue;
    }
    if (strcmp(record->requested_url, url) == 0) {
      if (seen == index) {
        return record;
      }
      seen += 1;
    }
  }
  return NULL;
}

int mln_test_provider_requests(mln_test_provider* provider, const char* url) {
  int count = 0;
  while (mln_test_provider_request_at(provider, url, count) != NULL) {
    count += 1;
  }
  return count;
}

typedef struct provider_count_target {
  mln_test_provider* provider;
  const char* url;
  int count;
} provider_count_target;

static bool provider_count_reached(void* context) {
  const provider_count_target* target = context;
  return mln_test_provider_requests(target->provider, target->url) >=
         target->count;
}

bool mln_test_provider_wait_for_requests(
  mln_test_provider* provider, const char* url, int count
) {
  provider_count_target target = {
    .provider = provider, .url = url, .count = count
  };
  return mln_test_await(
    provider_count_reached, &target, mln_test_deadline_default(),
    "a request to the scripted provider"
  );
}
