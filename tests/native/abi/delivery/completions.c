// Queue-mode completions: what a delivery copies, descriptor validation, owned
// results, and completions whose queue the host released.

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

static bool view_contains(mln_buffer_view view, const char* needle) {
  const size_t length = strlen(needle);
  const char* bytes = view.data;
  for (size_t offset = 0; offset + length <= view.size; offset += 1) {
    if (memcmp(bytes + offset, needle, length) == 0) return true;
  }
  return false;
}

static const mln_completion_result* await_completion(
  mln_delivery_queue queue, void* user_data, mln_delivery_batch* out_batch
) {
  const mln_delivery* delivery = NULL;
  *out_batch = mln_test_delivery_await_single(queue, &delivery);
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_COMPLETION, delivery->type);
  TEST_ASSERT_EQUAL_PTR(user_data, delivery->user_data);
  TEST_ASSERT_EQUAL_UINT32(
    sizeof(mln_completion_result), delivery->payload.completion.size
  );
  return &delivery->payload.completion;
}

static void operation_and_command_completions_reach_the_queue(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  int operation_tag = 0;
  int command_tag = 0;

  mln_completion completion =
    mln_test_queue_completion(queue.handle, &operation_tag);
  MLN_TEST_OK(mln_runtime_clear_resource_transform(runtime, &completion, NULL));
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  const mln_completion_result* result =
    await_completion(queue.handle, &operation_tag, &batch);
  MLN_TEST_OK(result->status);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, result->disposition
  );
  TEST_ASSERT_EQUAL_UINT64(0, result->generation);
  TEST_ASSERT_NULL(result->value);
  TEST_ASSERT_EQUAL_size_t(0, result->diagnostic.size);
  mln_delivery_batch_release(batch);

  completion = mln_test_queue_completion(queue.handle, &command_tag);
  MLN_TEST_OK(mln_map_set_style_json(
    map, mln_test_background_style_json, &completion, NULL
  ));
  result = await_completion(queue.handle, &command_tag, &batch);
  MLN_TEST_OK(result->status);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, result->disposition
  );
  TEST_ASSERT_TRUE(result->generation > 0);
  mln_delivery_batch_release(batch);

  // A failed command copies its status and diagnostic.
  completion = mln_test_queue_completion(queue.handle, &command_tag);
  MLN_TEST_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("absent-layer"), &completion, NULL
  ));
  result = await_completion(queue.handle, &command_tag, &batch);
  MLN_TEST_STATUS(MLN_STATUS_NOT_FOUND, result->status);
  TEST_ASSERT_EQUAL_UINT32(MLN_COMMAND_DISPOSITION_FAILED, result->disposition);
  TEST_ASSERT_TRUE(view_contains(result->diagnostic, "layer does not exist"));
  mln_delivery_batch_release(batch);

  mln_test_destroy_map(map);
  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);
}

static void a_rejected_submission_queues_nothing(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_queue_completion(queue.handle, NULL);

  MLN_TEST_INVALID(
    mln_runtime_clear_resource_transform(MLN_HANDLE_NULL, &completion, NULL)
  );

  // A drained completion with a release, or neither a callback nor a queue.
  mln_test_completion callback = mln_test_completion_default(0);
  completion = callback.descriptor;
  completion.callback = NULL;
  completion.queue = queue.handle;
  MLN_TEST_INVALID(
    mln_runtime_clear_resource_transform(runtime, &completion, NULL)
  );
  completion = (mln_completion){.size = sizeof(mln_completion)};
  MLN_TEST_INVALID(
    mln_runtime_clear_resource_transform(runtime, &completion, NULL)
  );
  mln_test_completion_reject(&callback);
  mln_test_completion_destroy(&callback);

  // Queue mode carries no release callback.
  completion = mln_test_queue_completion(queue.handle, NULL);
  completion.release_user_data = mln_test_delivery_count_wake_release;
  MLN_TEST_INVALID(
    mln_runtime_clear_resource_transform(runtime, &completion, NULL)
  );

  // A handle of another type is invalid.
  completion = mln_test_queue_completion(runtime, NULL);
  MLN_TEST_INVALID(
    mln_runtime_clear_resource_transform(runtime, &completion, NULL)
  );

  // Rejected submissions return before accepting any work or completion.
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&queue.wakes));
  mln_test_delivery_drain drain;
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY, mln_test_delivery_drain_into(queue.handle, &drain)
  );

  // A released queue reports as a released handle.
  mln_test_delivery_queue_release(&queue);
  completion = mln_test_queue_completion(queue.handle, NULL);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_runtime_clear_resource_transform(runtime, &completion, NULL)
  );
  mln_test_destroy_runtime(runtime);
}

// A command accepts a null completion, but a present one still names exactly
// one of a callback and a queue.
static void a_command_validates_a_present_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_completion completion = {.size = sizeof(mln_completion)};
  MLN_TEST_INVALID(mln_map_set_debug_options(map, 0, &completion, NULL));
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, NULL, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A later command's completion reaches the queue after every earlier
// submission's on the same map, including across a discarded command.
static void a_later_completion_follows_earlier_queued_completions(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  int command_tag = 0;
  int later_tag = 0;

  const mln_completion command =
    mln_test_queue_completion(queue.handle, &command_tag);
  MLN_TEST_OK(
    mln_map_set_style_json(map, mln_test_background_style_json, &command, NULL)
  );
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, NULL, NULL));
  const mln_completion later =
    mln_test_queue_completion(queue.handle, &later_tag);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &later, NULL));

  void* order[2] = {NULL, NULL};
  size_t seen = 0;
  while (seen < 2) {
    const mln_test_delivery_drain drain =
      mln_test_delivery_await_batch(queue.handle);
    for (size_t index = 0; index < drain.view.delivery_count; index += 1) {
      const mln_delivery* delivery = mln_test_delivery_at(&drain.view, index);
      TEST_ASSERT_TRUE(seen < 2);
      TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_COMPLETION, delivery->type);
      MLN_TEST_OK(delivery->payload.completion.status);
      order[seen++] = delivery->user_data;
    }
    mln_delivery_batch_release(drain.batch);
  }
  TEST_ASSERT_EQUAL_PTR(&command_tag, order[0]);
  TEST_ASSERT_EQUAL_PTR(&later_tag, order[1]);

  mln_test_destroy_map(map);
  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);
}

// A list result's strings live in the batch, past the map and runtime that
// produced them.
static void list_results_outlive_the_runtime(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);

  const mln_completion completion =
    mln_test_queue_completion(queue.handle, NULL);
  MLN_TEST_OK(mln_map_list_style_layers(map, &completion, NULL));
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  const mln_completion_result* result =
    await_completion(queue.handle, NULL, &batch);
  MLN_TEST_OK(result->status);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);

  TEST_ASSERT_EQUAL_size_t(1, result->value_count);
  const mln_style_layer_info* layer = result->value;
  TEST_ASSERT_EQUAL_size_t(strlen("background"), layer->id.size);
  TEST_ASSERT_EQUAL_MEMORY("background", layer->id.data, layer->id.size);
  TEST_ASSERT_EQUAL_MEMORY("background", layer->type.data, layer->type.size);
  mln_delivery_batch_release(batch);
}

// The runtime's own release completes through the queue, after its threads
// are gone.
static void runtime_release_completes_through_the_queue(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  // Created outside the fixtures, which would release it again.
  const mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  int tag = 0;
  const mln_completion completion =
    mln_test_queue_completion(queue.handle, &tag);
  MLN_TEST_OK(mln_runtime_release(runtime, &completion, NULL));
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  const mln_completion_result* result =
    await_completion(queue.handle, &tag, &batch);
  MLN_TEST_OK(result->status);
  mln_delivery_batch_release(batch);
  mln_test_delivery_queue_release(&queue);
}

// Gates the runtime worker so a query's completion arrives only after the
// test releases the queue it names.
static void a_completion_for_a_released_queue_is_discarded(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map parking = mln_test_create_map(runtime);
  mln_test_gate gate;
  mln_test_gate_init(&gate);
  const mln_completion parked = mln_test_gate_completion(&gate);
  MLN_TEST_OK(mln_map_set_debug_options(parking, 0, &parked, NULL));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(&gate), "the runtime worker never parked"
  );

  const mln_completion completion =
    mln_test_queue_completion(queue.handle, NULL);
  MLN_TEST_OK(mln_map_get_style_url(parking, &completion, NULL));
  mln_test_delivery_queue_release(&queue);
  mln_test_gate_release(&gate);

  // A later query on the same worker runs after the refused completion.
  MLN_TEST_OK(mln_test_map_fence(parking));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&queue.wakes));
  mln_test_destroy_map(parking);
  mln_test_destroy_runtime(runtime);
}

static void runtime_release_can_finish_before_queued_results_are_drained(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  const mln_completion queued = mln_test_queue_completion(queue.handle, NULL);
  MLN_TEST_OK(mln_runtime_clear_resource_transform(runtime, &queued, NULL));
  MLN_TEST_AWAIT_OK(mln_runtime_release(runtime, &completion.descriptor, NULL));
  mln_test_delivery_drain drain;
  MLN_TEST_OK(mln_test_delivery_drain_into(queue.handle, &drain));
  const size_t count = drain.view.delivery_count;
  mln_delivery_batch_release(drain.batch);
  mln_test_delivery_queue_release(&queue);
  TEST_ASSERT_EQUAL_size_t(1, count);
}

MLN_TEST_GROUP {
  RUN_TEST(operation_and_command_completions_reach_the_queue);
  RUN_TEST(a_rejected_submission_queues_nothing);
  RUN_TEST(a_command_validates_a_present_completion);
  RUN_TEST(a_later_completion_follows_earlier_queued_completions);
  RUN_TEST(list_results_outlive_the_runtime);
  RUN_TEST(runtime_release_completes_through_the_queue);
  RUN_TEST(runtime_release_can_finish_before_queued_results_are_drained);
  RUN_TEST(a_completion_for_a_released_queue_is_discarded);
}
