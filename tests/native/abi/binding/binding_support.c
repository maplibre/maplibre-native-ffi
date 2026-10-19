// Binding support: Dart ports and wakes, which post through the post_cobject
// function that the host passes, so a fake of Dart's NativeApi.postCObject
// records the messages without a Dart VM. A message is an integer, or an array
// of integers in the Dart_CObject layout of native API version 2.

#include "maplibre_native_c/binding_support.h"

#include "support/delivery.h"
#include "support/test_support.h"

// The Dart_CObject types and union members that binding support posts, and
// the native pointer member that a decoder must still step over.
enum {
  dart_int64 = 3,
  dart_array = 6,
  dart_native_pointer = 11,
};

typedef void (*dart_finalizer)(void* isolate_callback_data, void* peer);

typedef struct dart_cobject {
  int32_t type;
  union {
    int64_t as_int64;
    struct {
      intptr_t length;
      struct dart_cobject** values;
    } as_array;
    struct {
      intptr_t ptr;
      intptr_t size;
      dart_finalizer callback;
    } as_native_pointer;
  } value;
} dart_cobject;

// One posted message, decoded. An array element is an integer, or a native
// pointer with its finalizer.
typedef struct posted_element {
  int32_t type;
  int64_t integer;
  void* pointer;
  dart_finalizer finalizer;
} posted_element;

typedef struct posted_message {
  int64_t port;
  int32_t type;
  size_t length;
  posted_element elements[4];
} posted_message;

enum { posted_capacity = 16 };

// Posts may come from MapLibre threads. A poster claims a slot, fills it, and
// then marks it ready.
static posted_message posted[posted_capacity];
static atomic_bool posted_ready[posted_capacity];
static atomic_size_t posted_count;
// What the fake reports: true delivers, false stands for a closed port.
static atomic_bool delivers;

static posted_element decode_element(const dart_cobject* object) {
  posted_element element = {.type = object->type};
  if (object->type == dart_int64) {
    element.integer = object->value.as_int64;
  } else if (object->type == dart_native_pointer) {
    element.pointer = (void*)object->value.as_native_pointer.ptr;
    element.finalizer = object->value.as_native_pointer.callback;
  }
  return element;
}

static bool fake_post_cobject(int64_t port, dart_cobject* message) {
  const size_t slot = atomic_fetch_add(&posted_count, 1);
  if (slot < posted_capacity) {
    posted_message* record = &posted[slot];
    record->port = port;
    record->type = message->type;
    if (message->type == dart_array) {
      const intptr_t length = message->value.as_array.length;
      record->length = (size_t)length;
      for (intptr_t index = 0; index < length && index < 4; ++index) {
        record->elements[index] =
          decode_element(message->value.as_array.values[index]);
      }
    } else {
      record->length = 1;
      record->elements[0] = decode_element(message);
    }
    atomic_store(&posted_ready[slot], true);
  }
  mln_test_pulse();
  return atomic_load(&delivers);
}

static void* fake_post_address(void) {
  bool (*function)(int64_t, dart_cobject*) = fake_post_cobject;
  void* address = NULL;
  memcpy(&address, &function, sizeof(address));
  return address;
}

static void reset_posts(bool deliver) {
  memset(posted, 0, sizeof(posted));
  for (size_t index = 0; index < posted_capacity; ++index) {
    atomic_store(&posted_ready[index], false);
  }
  atomic_store(&posted_count, 0);
  atomic_store(&delivers, deliver);
}

static bool message_ready(void* context) {
  return atomic_load(&posted_ready[*(const size_t*)context]);
}

// Waits for the message in one slot and returns it.
static const posted_message* await_message(size_t slot) {
  TEST_ASSERT_LESS_THAN_size_t(posted_capacity, slot);
  TEST_ASSERT_TRUE(mln_test_await(
    message_ready, &slot, mln_test_deadline_default(), "Dart port message"
  ));
  return &posted[slot];
}

static void assert_integer_message(
  size_t slot, int64_t port, int64_t expected
) {
  const posted_message* message = await_message(slot);
  TEST_ASSERT_EQUAL_INT64(port, message->port);
  TEST_ASSERT_EQUAL_INT32(dart_int64, message->type);
  TEST_ASSERT_EQUAL_INT64(expected, message->elements[0].integer);
}

// A wake posts 0 for each call and 1 when it is released.
static void dart_wakes_post_zero_per_wake_and_one_on_release(void) {
  reset_posts(true);
  mln_wake wake = {0};
  MLN_TEST_INVALID(mln_binding_dart_wake_create(NULL, 17, &wake, NULL));
  MLN_TEST_INVALID(
    mln_binding_dart_wake_create(fake_post_address(), 0, &wake, NULL)
  );
  MLN_TEST_OK(
    mln_binding_dart_wake_create(fake_post_address(), 17, &wake, NULL)
  );
  wake.callback(wake.user_data);
  wake.callback(wake.user_data);
  wake.release_user_data(wake.user_data);
  TEST_ASSERT_EQUAL_size_t(3, atomic_load(&posted_count));
  assert_integer_message(0, 17, 0);
  assert_integer_message(1, 17, 0);
  assert_integer_message(2, 17, 1);
}

// A Dart wake serves a delivery queue: a push into the empty queue posts 0,
// and releasing the queue posts 1 once the wake is gone.
static void a_dart_wake_serves_a_delivery_queue(void) {
  reset_posts(true);
  mln_delivery_queue_options options = {
    .size = sizeof(mln_delivery_queue_options)
  };
  MLN_TEST_OK(
    mln_binding_dart_wake_create(fake_post_address(), 41, &options.wake, NULL)
  );
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_delivery_queue_create(&options, &queue, NULL));
  mln_runtime runtime = mln_test_create_runtime();
  const mln_completion completion = mln_test_queue_completion(queue, NULL);
  MLN_TEST_OK(mln_runtime_barrier(runtime, &completion, NULL));
  assert_integer_message(0, 41, 0);

  mln_delivery_queue_release(queue);
  assert_integer_message(1, 41, 1);
  mln_test_destroy_runtime(runtime);
}

// A notification port posts the callback identifier and the copied arguments
// until it is released, and release posts 0 once.
static void dart_notification_ports_post_copied_arguments_until_released(void) {
  reset_posts(true);
  TEST_ASSERT_NULL(mln_binding_dart_port_create(NULL, 19));
  TEST_ASSERT_NULL(mln_binding_dart_port_create(fake_post_address(), 0));
  TEST_ASSERT_NULL(mln_binding_dart_port_function(0));
  void* context = mln_binding_dart_port_create(fake_post_address(), 19);
  TEST_ASSERT_NOT_NULL(context);
  void* address = mln_binding_dart_port_function(
    MLN_BINDING_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE
  );
  TEST_ASSERT_NOT_NULL(address);
  mln_custom_source_tile_callback fetch_tile = NULL;
  memcpy(&fetch_tile, &address, sizeof(fetch_tile));

  fetch_tile(context, (mln_canonical_tile_id){.z = 5, .x = 7, .y = 9});
  mln_binding_dart_port_release(context);
  mln_binding_dart_port_release(context);
  fetch_tile(context, (mln_canonical_tile_id){.z = 1, .x = 1, .y = 1});
  TEST_ASSERT_EQUAL_size_t(2, atomic_load(&posted_count));

  const posted_message* tile = await_message(0);
  TEST_ASSERT_EQUAL_INT64(19, tile->port);
  TEST_ASSERT_EQUAL_INT32(dart_array, tile->type);
  TEST_ASSERT_EQUAL_size_t(4, tile->length);
  const int64_t expected[] = {
    MLN_BINDING_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE, 5, 7, 9
  };
  for (size_t index = 0; index < 4; ++index) {
    TEST_ASSERT_EQUAL_INT32(dart_int64, tile->elements[index].type);
    TEST_ASSERT_EQUAL_INT64(expected[index], tile->elements[index].integer);
  }
  assert_integer_message(1, 19, 0);
}

// Creates a map that the handle fixtures do not record, for a case that hands
// it to an owner token.
static mln_map create_untracked_map(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(sizeof(mln_map));
  MLN_TEST_OK(
    mln_runtime_create_map(runtime, NULL, &completion.descriptor, NULL)
  );
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_completion_finish_value(&completion, &map, sizeof(map)));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  return map;
}

// Destroying a token after the host released its owner explicitly leaves the
// owner alone. Finalizing a token disposes the owner, a delivery queue
// included.
static void an_owner_token_disposes_its_owner_only_when_finalized(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map released = create_untracked_map(runtime);
  mln_map finalized = create_untracked_map(runtime);

  void* token = mln_binding_owner_token_create(released);
  TEST_ASSERT_NOT_NULL(token);
  mln_binding_owner_token_destroy(token);
  TEST_ASSERT_TRUE(mln_test_map_is_live(released));
  mln_test_destroy_map(released);

  token = mln_binding_owner_token_create(finalized);
  TEST_ASSERT_NOT_NULL(token);
  mln_binding_owner_finalize(token);
  TEST_ASSERT_FALSE(mln_test_map_is_live(finalized));

  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  token = mln_binding_owner_token_create(queue.handle);
  TEST_ASSERT_NOT_NULL(token);
  mln_binding_owner_finalize(token);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wake_releases, 1));
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_INVALID_STATE(mln_delivery_queue_drain(queue.handle, &batch, NULL));
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(dart_wakes_post_zero_per_wake_and_one_on_release);
  RUN_TEST(a_dart_wake_serves_a_delivery_queue);
  RUN_TEST(dart_notification_ports_post_copied_arguments_until_released);
  RUN_TEST(an_owner_token_disposes_its_owner_only_when_finalized);
}
