#ifndef MLN_NATIVE_TESTS_ADAPTER_H
#define MLN_NATIVE_TESTS_ADAPTER_H

// Helpers that the callback adapter cases share: a completion listener that
// keeps its record for the test thread, committed resource provider changes,
// and a map that no fixture tracks, for cases in which the adapter disposes it.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "env.h"
#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"
#include "unity.h"
#include "wait.h"

// What one adapter completion delivered. The listener runs on the thread that
// completes the call, so every field is atomic.
typedef struct mln_test_adapter_delivery {
  atomic_size_t deliveries;
  atomic_bool delivered;
  _Atomic(mln_adapter_completion_record*) record;
} mln_test_adapter_delivery;

// An mln_adapter_completion_listener that keeps its record, which may be null,
// in the delivery that user_data points to.
static inline void mln_test_adapter_keep_record(
  void* user_data, mln_adapter_completion_record* record
) {
  mln_test_adapter_delivery* delivery = user_data;
  atomic_store(&delivery->record, record);
  atomic_fetch_add(&delivery->deliveries, 1);
  mln_test_flag_set(&delivery->delivered);
}

// Creates an adapter completion whose listener keeps its record in delivery.
static inline mln_completion mln_test_adapter_completion(
  uint32_t copy_kind, size_t element_size, mln_test_adapter_delivery* delivery
) {
  mln_completion completion = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_completion_create(
                     copy_kind, element_size, mln_test_adapter_keep_record,
                     delivery, &completion, NULL
                   )
  );
  return completion;
}

// Submits a resource provider change and waits for it to commit.
static inline mln_status mln_test_adapter_commit(
  mln_status status, mln_test_completion* completion
) {
  if (status != MLN_STATUS_OK) {
    mln_test_completion_reject(completion);
    mln_test_completion_destroy(completion);
    return status;
  }
  return mln_test_completion_settle(completion);
}

static inline mln_status mln_test_adapter_set_provider(
  mln_runtime runtime, const mln_resource_provider* provider
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return mln_test_adapter_commit(
    mln_runtime_set_resource_provider(
      runtime, provider, &completion.descriptor, NULL
    ),
    &completion
  );
}

static inline mln_status mln_test_adapter_clear_provider(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  return mln_test_adapter_commit(
    mln_runtime_clear_resource_provider(runtime, &completion.descriptor, NULL),
    &completion
  );
}

// Creates a map that the handle fixtures do not record, for a case that hands
// it to an adapter owner. The case releases it, or checks that the adapter
// disposed it, before destroying the runtime.
static inline mln_map mln_test_adapter_create_map(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(sizeof(mln_map));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_create(runtime, NULL, &completion.descriptor, NULL)
  );
  mln_map map = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &map, sizeof(map))
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  return map;
}

// Reports whether a map handle is still live. Disposal consumes a handle before
// it returns, so a disposed map answers as stale at once.
static inline bool mln_test_adapter_map_is_live(mln_map map) {
  return mln_test_map_request_repaint(map) == MLN_STATUS_OK;
}

#endif
