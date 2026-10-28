// Queued completions: what a dispatched completion receives, descriptor
// validation, results that outlive their producer, and completions whose
// queue the host released.

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

// What one dispatched completion received, copied during the call.
typedef struct captured_result {
  size_t calls;
  void* user_data;
  uint32_t size;
  mln_status status;
  mln_command_disposition disposition;
  uint64_t generation;
  bool has_value;
  size_t value_count;
  char diagnostic[256];
  // The first layer of a style layer list, for the list case.
  char layer_id[32];
  char layer_type[32];
} captured_result;

static void copy_text(char* out, size_t capacity, mln_buffer_view text) {
  const size_t size = text.size < capacity - 1 ? text.size : capacity - 1;
  if (size != 0) memcpy(out, text.data, size);
  out[size] = '\0';
}

static captured_result* capture_target;

static void capture_result(
  void* user_data, const mln_completion_result* result
) {
  captured_result* captured = capture_target;
  captured->calls += 1;
  captured->user_data = user_data;
  captured->size = result->size;
  captured->status = result->status;
  captured->disposition = result->disposition;
  captured->generation = result->generation;
  captured->has_value = result->value != NULL;
  captured->value_count = result->value_count;
  copy_text(
    captured->diagnostic, sizeof(captured->diagnostic), result->diagnostic
  );
}

static void capture_layers(
  void* user_data, const mln_completion_result* result
) {
  capture_result(user_data, result);
  if (result->status != MLN_STATUS_OK || result->value_count == 0) return;
  const mln_style_layer_info* layer = result->value;
  copy_text(
    capture_target->layer_id, sizeof(capture_target->layer_id), layer->id
  );
  copy_text(
    capture_target->layer_type, sizeof(capture_target->layer_type), layer->type
  );
}

static mln_completion capturing_completion(
  mln_delivery_queue queue, void* user_data, captured_result* captured,
  mln_completion_callback callback
) {
  memset(captured, 0, sizeof(*captured));
  capture_target = captured;
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = callback,
    .user_data = user_data,
    .queue = queue,
  };
}

typedef struct capture_wait {
  mln_delivery_queue queue;
  const captured_result* captured;
  mln_status failure;
} capture_wait;

static bool captured_a_result(void* context) {
  capture_wait* wait = context;
  const mln_status status = mln_delivery_queue_dispatch(wait->queue, NULL);
  if (status != MLN_STATUS_OK && status != MLN_STATUS_NOT_READY) {
    wait->failure = status;
    return true;
  }
  return wait->captured->calls != 0;
}

// Dispatches queue until the capturing completion has run once.
static void await_capture(
  mln_delivery_queue queue, const captured_result* captured, void* user_data
) {
  capture_wait wait = {.queue = queue, .captured = captured};
  TEST_ASSERT_TRUE(mln_test_await(
    captured_a_result, &wait, mln_test_deadline_default(), "a completion"
  ));
  MLN_TEST_OK(wait.failure);
  TEST_ASSERT_EQUAL_size_t(1, captured->calls);
  TEST_ASSERT_EQUAL_PTR(user_data, captured->user_data);
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_completion_result), captured->size);
}

static void operation_and_command_completions_reach_the_queue(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  int operation_tag = 0;
  int command_tag = 0;
  captured_result captured;

  mln_completion completion = capturing_completion(
    queue.handle, &operation_tag, &captured, capture_result
  );
  MLN_TEST_OK(mln_runtime_clear_resource_transform(runtime, &completion, NULL));
  await_capture(queue.handle, &captured, &operation_tag);
  MLN_TEST_OK(captured.status);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, captured.disposition
  );
  TEST_ASSERT_EQUAL_UINT64(0, captured.generation);
  TEST_ASSERT_FALSE(captured.has_value);
  TEST_ASSERT_EQUAL_STRING("", captured.diagnostic);

  completion =
    capturing_completion(queue.handle, &command_tag, &captured, capture_result);
  MLN_TEST_OK(mln_map_set_style_json(
    map, mln_test_background_style_json, &completion, NULL
  ));
  await_capture(queue.handle, &captured, &command_tag);
  MLN_TEST_OK(captured.status);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, captured.disposition
  );
  TEST_ASSERT_TRUE(captured.generation > 0);

  // A failed command copies its status and diagnostic.
  completion =
    capturing_completion(queue.handle, &command_tag, &captured, capture_result);
  MLN_TEST_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("absent-layer"), &completion, NULL
  ));
  await_capture(queue.handle, &captured, &command_tag);
  MLN_TEST_STATUS(MLN_STATUS_NOT_FOUND, captured.status);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_FAILED, captured.disposition
  );
  TEST_ASSERT_NOT_NULL(strstr(captured.diagnostic, "layer does not exist"));

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

  // A queue does not stand in for the callback.
  completion.callback = NULL;
  MLN_TEST_INVALID(
    mln_runtime_clear_resource_transform(runtime, &completion, NULL)
  );
  completion = (mln_completion){.size = sizeof(mln_completion)};
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
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY, mln_delivery_queue_dispatch(queue.handle, NULL)
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

// A command accepts a null completion, but a present one still names a
// callback.
static void a_command_validates_a_present_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_completion completion = {.size = sizeof(mln_completion)};
  MLN_TEST_INVALID(mln_map_set_debug_options(map, 0, &completion, NULL));
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, NULL, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A later command's completion runs after every earlier submission's on the
// same map, including across a discarded command.
static void a_later_completion_follows_earlier_queued_completions(void) {
  mln_test_calls_reset();
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

  mln_test_dispatch_until_calls(queue.handle, 2);
  TEST_ASSERT_EQUAL_size_t(2, mln_test_calls.count);
  TEST_ASSERT_EQUAL_PTR(&command_tag, mln_test_call_at(0)->user_data);
  MLN_TEST_OK(mln_test_call_at(0)->status);
  TEST_ASSERT_EQUAL_PTR(&later_tag, mln_test_call_at(1)->user_data);
  MLN_TEST_OK(mln_test_call_at(1)->status);

  mln_test_destroy_map(map);
  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);
}

// A list result's strings live in the queued call, past the map and runtime
// that produced them.
static void list_results_outlive_the_runtime(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);

  captured_result captured;
  const mln_completion completion =
    capturing_completion(queue.handle, NULL, &captured, capture_layers);
  MLN_TEST_OK(mln_map_list_style_layers(map, &completion, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, 1));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  await_capture(queue.handle, &captured, NULL);
  MLN_TEST_OK(captured.status);
  TEST_ASSERT_EQUAL_size_t(1, captured.value_count);
  TEST_ASSERT_EQUAL_STRING("background", captured.layer_id);
  TEST_ASSERT_EQUAL_STRING("background", captured.layer_type);
  mln_test_delivery_queue_release(&queue);
}

// The runtime's own release completes through the queue, after its threads
// are gone.
static void runtime_release_completes_through_the_queue(void) {
  mln_test_calls_reset();
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
  mln_test_dispatch_until_calls(queue.handle, 1);
  TEST_ASSERT_EQUAL_PTR(&tag, mln_test_call_at(0)->user_data);
  MLN_TEST_OK(mln_test_call_at(0)->status);
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

static void runtime_release_can_finish_before_queued_results_run(void) {
  mln_test_calls_reset();
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  const mln_completion queued = mln_test_queue_completion(queue.handle, NULL);
  MLN_TEST_OK(mln_runtime_clear_resource_transform(runtime, &queued, NULL));
  MLN_TEST_AWAIT_OK(mln_runtime_release(runtime, &completion.descriptor, NULL));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_calls.count);
  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  mln_test_delivery_queue_release(&queue);
  TEST_ASSERT_EQUAL_size_t(1, mln_test_calls.count);
}

MLN_TEST_GROUP {
  RUN_TEST(operation_and_command_completions_reach_the_queue);
  RUN_TEST(a_rejected_submission_queues_nothing);
  RUN_TEST(a_command_validates_a_present_completion);
  RUN_TEST(a_later_completion_follows_earlier_queued_completions);
  RUN_TEST(list_results_outlive_the_runtime);
  RUN_TEST(runtime_release_completes_through_the_queue);
  RUN_TEST(runtime_release_can_finish_before_queued_results_run);
  RUN_TEST(a_completion_for_a_released_queue_is_discarded);
}
