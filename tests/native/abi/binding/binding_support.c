// Binding support: Dart wakes, which post through the post_cobject function
// that the host passes, so a fake of Dart's NativeApi.postCObject records the
// messages without a Dart VM. A message is an integer in the Dart_CObject
// layout of native API version 2; and owner tokens.

#include "maplibre_native_c/binding_support.h"

#include "support/delivery.h"
#include "support/test_support.h"

// The Dart_CObject integer type that a wake posts.
enum { dart_int64 = 3 };

typedef struct dart_cobject {
  int32_t type;
  union {
    int64_t as_int64;
  } value;
} dart_cobject;

typedef struct posted_message {
  int64_t port;
  int32_t type;
  int64_t integer;
} posted_message;

enum { posted_capacity = 16 };

static posted_message posted[posted_capacity];
static atomic_size_t posted_count;
// What the fake reports: true delivers, false stands for a closed port.
static atomic_bool delivers;

static bool fake_post_cobject(int64_t port, dart_cobject* message) {
  const size_t slot = atomic_fetch_add(&posted_count, 1);
  if (slot < posted_capacity) {
    posted[slot] = (posted_message){
      .port = port,
      .type = message->type,
      .integer = message->value.as_int64,
    };
  }
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
  atomic_store(&posted_count, 0);
  atomic_store(&delivers, deliver);
}

static void assert_integer_message(
  size_t slot, int64_t port, int64_t expected
) {
  const posted_message* message = &posted[slot];
  TEST_ASSERT_EQUAL_INT64(port, message->port);
  TEST_ASSERT_EQUAL_INT32(dart_int64, message->type);
  TEST_ASSERT_EQUAL_INT64(expected, message->integer);
}

// A wake posts 0 for each call and 1 when it is released. A closed port that
// rejects every post changes nothing: the wake still posts, and its release
// still frees the context.
static void dart_wakes_post_zero_per_wake_and_one_on_release(void) {
  mln_wake wake = {0};
  MLN_TEST_INVALID(mln_binding_dart_wake_create(NULL, 17, &wake, NULL));
  MLN_TEST_INVALID(
    mln_binding_dart_wake_create(fake_post_address(), 0, &wake, NULL)
  );
  for (int open = 1; open >= 0; open -= 1) {
    reset_posts(open == 1);
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
  mln_test_delivery_drain drain;
  MLN_TEST_INVALID_STATE(mln_test_delivery_drain_into(queue.handle, &drain));
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(dart_wakes_post_zero_per_wake_and_one_on_release);
  RUN_TEST(an_owner_token_disposes_its_owner_only_when_finalized);
}
