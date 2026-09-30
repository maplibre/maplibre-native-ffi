// Dart ports. Each Dart entry point posts through the post_cobject function
// that the host passes, so a fake of Dart's NativeApi.postCObject records the
// messages without a Dart VM. A message is an integer, or an array of integers
// and native pointers in the Dart_CObject layout of native API version 2.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

// The Dart_CObject types and union members that the adapter posts.
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

// Checks a message of an integer and a native pointer with a finalizer, and
// returns the pointer.
static void* assert_pointer_message(
  size_t slot, int64_t port, int64_t expected, dart_finalizer* out_finalizer
) {
  const posted_message* message = await_message(slot);
  TEST_ASSERT_EQUAL_INT64(port, message->port);
  TEST_ASSERT_EQUAL_INT32(dart_array, message->type);
  TEST_ASSERT_EQUAL_size_t(2, message->length);
  TEST_ASSERT_EQUAL_INT32(dart_int64, message->elements[0].type);
  TEST_ASSERT_EQUAL_INT64(expected, message->elements[0].integer);
  TEST_ASSERT_EQUAL_INT32(dart_native_pointer, message->elements[1].type);
  TEST_ASSERT_NOT_NULL(message->elements[1].pointer);
  TEST_ASSERT_NOT_NULL(message->elements[1].finalizer);
  *out_finalizer = message->elements[1].finalizer;
  return message->elements[1].pointer;
}

// A wake posts 0 for each call and 1 when it is released.
static void dart_wakes_post_zero_per_wake_and_one_on_release(void) {
  reset_posts(true);
  mln_wake wake = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_dart_wake_create(NULL, 17, &wake, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_dart_wake_create(fake_post_address(), 0, &wake, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_adapter_dart_wake_create(fake_post_address(), 17, &wake, NULL)
  );
  wake.callback(wake.user_data);
  wake.callback(wake.user_data);
  wake.release_user_data(wake.user_data);
  TEST_ASSERT_EQUAL_size_t(3, atomic_load(&posted_count));
  assert_integer_message(0, 17, 0);
  assert_integer_message(1, 17, 0);
  assert_integer_message(2, 17, 1);
}

// Creates a map through a Dart completion and returns the posted record.
static mln_adapter_completion_record* post_map_record(
  mln_runtime runtime, dart_finalizer* out_finalizer, mln_map* out_map
) {
  mln_completion completion = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_dart_completion_create(
                     MLN_ADAPTER_COMPLETION_COPY_MAP, sizeof(mln_map),
                     fake_post_address(), 23, 31, &completion, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_create(runtime, NULL, &completion, NULL)
  );
  mln_adapter_completion_record* record =
    assert_pointer_message(0, 23, 31, out_finalizer);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, record->result.status);
  TEST_ASSERT_EQUAL_size_t(1, record->result.value_count);
  *out_map = *(const mln_map*)record->result.value;
  TEST_ASSERT_TRUE(mln_test_adapter_map_is_live(*out_map));
  return record;
}

// A delivered completion posts its token and a record that the binding's
// decoder adopts.
static void dart_completions_post_their_token_and_record(void) {
  reset_posts(true);
  mln_runtime runtime = mln_test_create_runtime();
  dart_finalizer finalizer = NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_adapter_completion_record* record =
    post_map_record(runtime, &finalizer, &map);

  mln_adapter_completion_record_adopt(record);
  mln_adapter_completion_record_destroy(record);
  TEST_ASSERT_TRUE(mln_test_adapter_map_is_live(map));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The VM runs the message finalizer of a completion that never reached the
// isolate, and the finalizer disposes the owned result.
static void an_undelivered_dart_completion_disposes_its_result(void) {
  reset_posts(false);
  mln_runtime runtime = mln_test_create_runtime();
  dart_finalizer finalizer = NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_adapter_completion_record* record =
    post_map_record(runtime, &finalizer, &map);

  finalizer(NULL, record);
  TEST_ASSERT_FALSE(mln_test_adapter_map_is_live(map));
  mln_test_destroy_runtime(runtime);
}

static void a_rejected_dart_completion_posts_nothing(void) {
  reset_posts(true);
  mln_completion completion = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_dart_completion_create(
      MLN_ADAPTER_COMPLETION_COPY_MAP, sizeof(mln_map), NULL, 23, 31,
      &completion, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_dart_completion_create(
                     MLN_ADAPTER_COMPLETION_COPY_MAP, sizeof(mln_map),
                     fake_post_address(), 23, 31, &completion, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(MLN_HANDLE_NULL, NULL, &completion, NULL)
  );
  mln_adapter_completion_reject(&completion);
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&posted_count));
}

static mln_log_callback deferred_log_callback(void) {
  void* address =
    mln_adapter_deferred_callback_function(MLN_ADAPTER_DEFERRED_LOG_CALLBACK);
  TEST_ASSERT_NOT_NULL(address);
  mln_log_callback callback = NULL;
  memcpy(&callback, &address, sizeof(callback));
  return callback;
}

static mln_resource_provider_callback deferred_provider_callback(void) {
  void* address = mln_adapter_deferred_callback_function(
    MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK
  );
  TEST_ASSERT_NOT_NULL(address);
  mln_resource_provider_callback callback = NULL;
  memcpy(&callback, &address, sizeof(callback));
  return callback;
}

static void* dart_deferred_context(uint32_t callback, int64_t port) {
  void* context = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_dart_deferred_callback_create(
                     callback, fake_post_address(), port, &context, NULL
                   )
  );
  TEST_ASSERT_NOT_NULL(context);
  return context;
}

// A deferred call posts its callback value and a copied record, and releasing
// the context posts 0.
static void dart_deferred_callbacks_post_the_callback_and_record(void) {
  reset_posts(true);
  void* context = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_dart_deferred_callback_create(
      MLN_ADAPTER_DEFERRED_LOG_CALLBACK, fake_post_address(), 0, &context, NULL
    )
  );
  TEST_ASSERT_NULL(context);
  context = dart_deferred_context(MLN_ADAPTER_DEFERRED_LOG_CALLBACK, 29);
  char message[] = "deferred";
  TEST_ASSERT_EQUAL_UINT32(
    1, deferred_log_callback()(
         context, MLN_LOG_SEVERITY_INFO, MLN_LOG_EVENT_GENERAL, 7, message
       )
  );
  memset(message, 'X', sizeof(message) - 1);

  dart_finalizer finalizer = NULL;
  mln_adapter_deferred_call_record* record = assert_pointer_message(
    0, 29, MLN_ADAPTER_DEFERRED_LOG_CALLBACK, &finalizer
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_ADAPTER_DEFERRED_LOG_CALLBACK, record->callback);
  const mln_adapter_log_callback_arguments* arguments = record->arguments;
  TEST_ASSERT_EQUAL_INT64(7, arguments->code);
  TEST_ASSERT_EQUAL_STRING("deferred", arguments->message);
  mln_adapter_deferred_call_record_destroy(record);

  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_EQUAL_size_t(2, atomic_load(&posted_count));
  assert_integer_message(1, 29, 0);
}

static bool wait_for_map_event(
  mln_runtime runtime, uint32_t type, mln_map map, char* message,
  size_t message_capacity
) {
  mln_runtime_event event = {0};
  return mln_test_await_event(
    runtime, type, map, &event, message, message_capacity
  );
}

// The finalizer of an undelivered provider record destroys it without
// adopting its decision handle, so the request fails instead of waiting.
static void an_undelivered_dart_provider_record_fails_its_request(void) {
  reset_posts(false);
  void* context =
    dart_deferred_context(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, 37);
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = deferred_provider_callback(),
    .user_data = context,
    .release_user_data = mln_adapter_deferred_callback_release,
  };
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_adapter_set_provider(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_url(map, "custom://dart-provider-style.json")
  );
  dart_finalizer finalizer = NULL;
  void* record = assert_pointer_message(
    0, 37, MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, &finalizer
  );
  finalizer(NULL, record);

  char message[256] = {0};
  TEST_ASSERT_TRUE(wait_for_map_event(
    runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, message, sizeof(message)
  ));
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));

  // Clearing the provider releases the context, which posts 0 last.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_adapter_clear_provider(runtime)
  );
  TEST_ASSERT_EQUAL_size_t(2, atomic_load(&posted_count));
  assert_integer_message(1, 37, 0);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A notification port posts the callback identifier and the copied arguments
// until it is released, and release posts 0 once.
static void dart_notification_ports_post_copied_arguments_until_released(void) {
  reset_posts(true);
  TEST_ASSERT_NULL(mln_adapter_dart_port_create(NULL, 19));
  TEST_ASSERT_NULL(mln_adapter_dart_port_create(fake_post_address(), 0));
  TEST_ASSERT_NULL(mln_adapter_dart_port_function(0));
  void* context = mln_adapter_dart_port_create(fake_post_address(), 19);
  TEST_ASSERT_NOT_NULL(context);
  void* address = mln_adapter_dart_port_function(
    MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE
  );
  TEST_ASSERT_NOT_NULL(address);
  mln_custom_geometry_source_tile_callback fetch_tile = NULL;
  memcpy(&fetch_tile, &address, sizeof(fetch_tile));

  fetch_tile(context, (mln_canonical_tile_id){.z = 5, .x = 7, .y = 9});
  mln_adapter_dart_port_release(context);
  mln_adapter_dart_port_release(context);
  fetch_tile(context, (mln_canonical_tile_id){.z = 1, .x = 1, .y = 1});
  TEST_ASSERT_EQUAL_size_t(2, atomic_load(&posted_count));

  const posted_message* tile = await_message(0);
  TEST_ASSERT_EQUAL_INT64(19, tile->port);
  TEST_ASSERT_EQUAL_INT32(dart_array, tile->type);
  TEST_ASSERT_EQUAL_size_t(4, tile->length);
  const int64_t expected[] = {
    MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE, 5, 7, 9
  };
  for (size_t index = 0; index < 4; ++index) {
    TEST_ASSERT_EQUAL_INT32(dart_int64, tile->elements[index].type);
    TEST_ASSERT_EQUAL_INT64(expected[index], tile->elements[index].integer);
  }
  assert_integer_message(1, 19, 0);
}

static unsigned registration_releases;

static void count_registration_release(void* context) {
  (void)context;
  registration_releases += 1;
}

static void* arena_with_counted_release(void) {
  void* arena = mln_adapter_arena_create();
  TEST_ASSERT_NOT_NULL(arena);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_arena_adopt_release(
                     arena, count_registration_release, NULL, NULL
                   )
  );
  return arena;
}

// A release registration posts its identifier once and destroys its arena,
// even after the isolate closed its port. A later registration of the same
// context address gets a new identifier.
static void dart_release_registrations_post_their_identifier_once(void) {
  reset_posts(false);
  registration_releases = 0;
  static int context;

  uint64_t first = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_dart_release_register(
                     fake_post_address(), 17, &context,
                     arena_with_counted_release(), &first, NULL
                   )
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(0, first);
  TEST_ASSERT_EQUAL_UINT(0, registration_releases);
  mln_adapter_dart_release(&context);
  mln_adapter_dart_release(&context);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&posted_count));
  assert_integer_message(0, 17, (int64_t)first);
  TEST_ASSERT_EQUAL_UINT(1, registration_releases);

  uint64_t second = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_dart_release_register(
                     fake_post_address(), 17, &context, NULL, &second, NULL
                   )
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(first, second);

  // A context holds one registration at a time. The rejected registration
  // still consumes its arena.
  uint64_t duplicate = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_dart_release_register(
      fake_post_address(), 17, &context, arena_with_counted_release(),
      &duplicate, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT64(0, duplicate);
  TEST_ASSERT_EQUAL_UINT(2, registration_releases);

  mln_adapter_dart_release(&context);
  TEST_ASSERT_EQUAL_size_t(2, atomic_load(&posted_count));
  assert_integer_message(1, 17, (int64_t)second);
}

MLN_TEST_GROUP {
  RUN_TEST(dart_wakes_post_zero_per_wake_and_one_on_release);
  RUN_TEST(dart_completions_post_their_token_and_record);
  RUN_TEST(an_undelivered_dart_completion_disposes_its_result);
  RUN_TEST(a_rejected_dart_completion_posts_nothing);
  RUN_TEST(dart_deferred_callbacks_post_the_callback_and_record);
  RUN_TEST(an_undelivered_dart_provider_record_fails_its_request);
  RUN_TEST(dart_notification_ports_post_copied_arguments_until_released);
  RUN_TEST(dart_release_registrations_post_their_identifier_once);
}
