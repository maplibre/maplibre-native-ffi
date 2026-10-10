// What the Vulkan driver does with the host's device and queue. Vulkan is the
// one backend that checks a context the submission accepted once the driver
// has it: the other backends reject a bad descriptor at submission or cannot
// fail after it. And a session shares its device with the host, so it waits on
// its own queue and never on the whole device, and a host that shares the
// queue with it gives it a lock that it takes around each call on the queue.

#include <vulkan/vulkan_core.h>

#include "mln_test_graphics.h"
#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

// A failed attach still published its session, which the host owns: it
// abandons and destroys the session, and the map's session slot comes back.
static void a_failed_attach_still_owns_the_session_it_published(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_graphics* graphics =
    mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_VULKAN);
  TEST_ASSERT_NOT_NULL_MESSAGE(graphics, mln_test_graphics_last_error());
  mln_test_graphics_context context = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(graphics, &context));

  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
  descriptor.extent = (mln_logical_extent){
    .width = 32,
    .height = 32,
    .scale_factor = 1.0,
  };
  descriptor.context = (mln_vulkan_context_descriptor){
    .instance = context.vulkan_instance,
    .physical_device = context.vulkan_physical_device,
    .device = context.vulkan_device,
    .graphics_queue = context.vulkan_queue,
    // Submission checks only that every handle is present; the driver
    // measures the family against the physical device.
    .graphics_queue_family_index = UINT32_MAX,
    .get_instance_proc_addr = context.vulkan_get_instance_proc_addr,
    .get_device_proc_addr = context.vulkan_get_device_proc_addr,
  };
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  options.requested_texture_ring_depth = 2;
  mln_render_session session = MLN_HANDLE_NULL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_attach_vulkan_owned_texture(
      map, &descriptor, &options, &session, &completion.descriptor, NULL
    )
  );

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(session, &snapshot, NULL));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_TARGET_LOST, snapshot.state
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_render_session_destroy(session, NULL)
  );
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(session, &abandoned, NULL));
  MLN_TEST_OK(mln_render_session_destroy(session, NULL));

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_graphics_destroy(graphics);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Watches the device functions a session resolves through its context's
// vkGetDeviceProcAddr. A case reads the counts only after a completion orders
// them after the driver's calls.
typedef struct vulkan_observer {
  PFN_vkGetDeviceProcAddr real_get_device_proc_addr;
  PFN_vkDeviceWaitIdle real_device_wait_idle;
  PFN_vkQueueSubmit real_queue_submit;
  PFN_vkDestroyFence real_destroy_fence;
  PFN_vkGetFenceStatus real_get_fence_status;
  atomic_uint device_waits;
  // Submissions with at least one batch.
  atomic_uint submissions;
  // Submissions of any kind from a thread that did not hold the host queue
  // lock, for a case whose session has one.
  atomic_uint unlocked_submissions;
  // Empty submissions with a fence, which drain the queue: the fence signals
  // once everything submitted to the queue before it has completed.
  atomic_uint drains;
  // Drain fences destroyed before they signaled, so the drain did not wait.
  atomic_uint unfinished_drains;
  // A session drains from one thread at a time, so one fence is pending.
  _Atomic(VkFence) drain_fence;
} vulkan_observer;

// One observer's own entry points, which route to it.
typedef struct vulkan_observer_entry_points {
  PFN_vkGetDeviceProcAddr get_device_proc_addr;
  PFN_vkDeviceWaitIdle device_wait_idle;
  PFN_vkQueueSubmit queue_submit;
  PFN_vkDestroyFence destroy_fence;
} vulkan_observer_entry_points;

static void observer_reset(vulkan_observer* observer) {
  atomic_store(&observer->device_waits, 0);
  atomic_store(&observer->submissions, 0);
  atomic_store(&observer->unlocked_submissions, 0);
  atomic_store(&observer->drains, 0);
  atomic_store(&observer->unfinished_drains, 0);
  atomic_store(&observer->drain_fence, VK_NULL_HANDLE);
}

static MLN_TEST_THREAD_LOCAL bool holds_host_queue_lock;

static VkResult observe_device_wait_idle(
  vulkan_observer* observer, VkDevice device
) {
  atomic_fetch_add(&observer->device_waits, 1);
  return observer->real_device_wait_idle(device);
}

static VkResult observe_queue_submit(
  vulkan_observer* observer, VkQueue queue, uint32_t submit_count,
  const VkSubmitInfo* submits, VkFence fence
) {
  if (!holds_host_queue_lock) {
    atomic_fetch_add(&observer->unlocked_submissions, 1);
  }
  if (submit_count == 0 && fence != VK_NULL_HANDLE) {
    atomic_fetch_add(&observer->drains, 1);
    atomic_store(&observer->drain_fence, fence);
  } else if (submit_count > 0) {
    atomic_fetch_add(&observer->submissions, 1);
  }
  return observer->real_queue_submit(queue, submit_count, submits, fence);
}

static void observe_destroy_fence(
  vulkan_observer* observer, VkDevice device, VkFence fence,
  const VkAllocationCallbacks* allocator
) {
  VkFence drain_fence = fence;
  if (
    fence != VK_NULL_HANDLE &&
    atomic_compare_exchange_strong(
      &observer->drain_fence, &drain_fence, VK_NULL_HANDLE
    ) &&
    observer->real_get_fence_status(device, fence) != VK_SUCCESS
  ) {
    atomic_fetch_add(&observer->unfinished_drains, 1);
  }
  observer->real_destroy_fence(device, fence, allocator);
}

static PFN_vkVoidFunction observe_get_device_proc_addr(
  vulkan_observer* observer, const vulkan_observer_entry_points* entry_points,
  VkDevice device, const char* name
) {
  // Like a layer, it answers for itself, because a dispatch table looks
  // vkGetDeviceProcAddr up through the one it was given.
  if (strcmp(name, "vkGetDeviceProcAddr") == 0) {
    return (PFN_vkVoidFunction)entry_points->get_device_proc_addr;
  }
  const PFN_vkVoidFunction real =
    observer->real_get_device_proc_addr(device, name);
  if (strcmp(name, "vkDeviceWaitIdle") == 0) {
    observer->real_device_wait_idle = (PFN_vkDeviceWaitIdle)real;
    return (PFN_vkVoidFunction)entry_points->device_wait_idle;
  }
  if (strcmp(name, "vkQueueSubmit") == 0) {
    observer->real_queue_submit = (PFN_vkQueueSubmit)real;
    return (PFN_vkVoidFunction)entry_points->queue_submit;
  }
  if (strcmp(name, "vkDestroyFence") == 0) {
    observer->real_destroy_fence = (PFN_vkDestroyFence)real;
    observer->real_get_fence_status =
      (PFN_vkGetFenceStatus)observer->real_get_device_proc_addr(
        device, "vkGetFenceStatus"
      );
    return (PFN_vkVoidFunction)entry_points->destroy_fence;
  }
  return real;
}

// Defines an observer `name` and its entry points. `name##_wrap` is the
// mln_test_vulkan_device_proc_addr_wrap that installs it.
#define VULKAN_OBSERVER(name)                                                  \
  static vulkan_observer name;                                                 \
  static VKAPI_ATTR VkResult VKAPI_CALL name##_device_wait_idle(               \
    VkDevice device                                                            \
  ) {                                                                          \
    return observe_device_wait_idle(&(name), device);                          \
  }                                                                            \
  static VKAPI_ATTR VkResult VKAPI_CALL name##_queue_submit(                   \
    VkQueue queue, uint32_t submit_count, const VkSubmitInfo* submits,         \
    VkFence fence                                                              \
  ) {                                                                          \
    return observe_queue_submit(&(name), queue, submit_count, submits, fence); \
  }                                                                            \
  static VKAPI_ATTR void VKAPI_CALL name##_destroy_fence(                      \
    VkDevice device, VkFence fence, const VkAllocationCallbacks* allocator     \
  ) {                                                                          \
    observe_destroy_fence(&(name), device, fence, allocator);                  \
  }                                                                            \
  static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL name##_get_device_proc_addr( \
    VkDevice device, const char* function                                      \
  ) {                                                                          \
    const vulkan_observer_entry_points entry_points = {                        \
      .get_device_proc_addr = name##_get_device_proc_addr,                     \
      .device_wait_idle = name##_device_wait_idle,                             \
      .queue_submit = name##_queue_submit,                                     \
      .destroy_fence = name##_destroy_fence,                                   \
    };                                                                         \
    return observe_get_device_proc_addr(                                       \
      &(name), &entry_points, device, function                                 \
    );                                                                         \
  }                                                                            \
  static void* name##_wrap(void* get_device_proc_addr) {                       \
    (name).real_get_device_proc_addr =                                         \
      (PFN_vkGetDeviceProcAddr)(uintptr_t)get_device_proc_addr;                \
    return (void*)(uintptr_t)name##_get_device_proc_addr;                      \
  }

VULKAN_OBSERVER(first_observer)
VULKAN_OBSERVER(second_observer)

static void render_one_frame(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
}

static void detach(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_detach(fixture->session, &completion.descriptor, NULL)
  );
}

// Asserts that teardown drained the session's queue through its own device
// functions, waiting for each drain to finish, and never waited on the device.
static void assert_drained_own_queue(vulkan_observer* observer) {
  TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&observer->drains));
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&observer->unfinished_drains));
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&observer->device_waits));
}

// A host compositor may submit on its own queue while a session renders, so a
// session never waits on the whole device, which would need every queue. Its
// teardown waits for its own work by draining its own queue instead.
static void a_session_drains_its_own_queue_and_never_waits_on_the_device(void) {
  observer_reset(&first_observer);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_borrowed_texture(
      map, &fixture, first_observer_wrap, NULL, NULL
    ),
    mln_test_graphics_last_error()
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, fixture.driver);
  render_one_frame(&fixture, 1);
  TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&first_observer.submissions));

  detach(&fixture);
  assert_drained_own_queue(&first_observer);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Two sessions given one queue each call through the device functions their
// own context resolved, so a host that interposes on those functions sees only
// its own session, even while another session shares the queue.
static void sessions_sharing_a_queue_call_their_own_device_functions(void) {
  observer_reset(&first_observer);
  observer_reset(&second_observer);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map first_map = mln_test_create_map(runtime);
  mln_map second_map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, first_map);
  mln_test_render_prepare_map(runtime, second_map);
  mln_test_render_fixture first = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_borrowed_texture(
      first_map, &first, first_observer_wrap, NULL, NULL
    ),
    mln_test_graphics_last_error()
  );
  mln_test_render_fixture second = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_borrowed_texture(
      second_map, &second, second_observer_wrap, &first, NULL
    ),
    mln_test_graphics_last_error()
  );
  render_one_frame(&first, 1);
  render_one_frame(&second, 1);
  TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&second_observer.submissions));

  // The second session leaves first, while the first still holds the queue.
  detach(&second);
  assert_drained_own_queue(&second_observer);
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&first_observer.drains));
  mln_test_render_fixture_destroy(&second);

  detach(&first);
  assert_drained_own_queue(&first_observer);
  mln_test_render_fixture_destroy(&first);
  mln_test_destroy_map(second_map);
  mln_test_destroy_map(first_map);
  mln_test_destroy_runtime(runtime);
}

// The host's lock on a queue that it shares with a session. C11 has no
// portable mutex, and each hold covers one queue call, so this is a ticket
// lock, which serves waiters in order and so starves neither thread.
typedef struct host_queue_lock {
  atomic_uint next_ticket;
  atomic_uint serving;
  // Calls the session made to its callbacks.
  atomic_uint locks;
  atomic_uint unlocks;
  atomic_uint releases;
} host_queue_lock;

static void host_queue_lock_take(host_queue_lock* lock) {
  const unsigned int ticket = atomic_fetch_add(&lock->next_ticket, 1);
  while (atomic_load(&lock->serving) != ticket) {
  }
  holds_host_queue_lock = true;
}

static void host_queue_lock_give(host_queue_lock* lock) {
  holds_host_queue_lock = false;
  atomic_fetch_add(&lock->serving, 1);
}

static void session_takes_queue_lock(void* user_data) {
  host_queue_lock* lock = user_data;
  host_queue_lock_take(lock);
  atomic_fetch_add(&lock->locks, 1);
}

static void session_gives_queue_lock(void* user_data) {
  host_queue_lock* lock = user_data;
  atomic_fetch_add(&lock->unlocks, 1);
  host_queue_lock_give(lock);
}

static void queue_lock_released(void* user_data) {
  host_queue_lock* lock = user_data;
  atomic_fetch_add(&lock->releases, 1);
}

// A host thread that submits empty batches on the shared queue under the lock
// until the case stops it.
typedef struct host_submitter {
  host_queue_lock* lock;
  PFN_vkQueueSubmit queue_submit;
  VkQueue queue;
  atomic_bool stop;
  atomic_int submissions;
  atomic_uint failures;
} host_submitter;

// File scope, because the host thread and the session's callbacks can outlast
// a case that fails partway.
static host_queue_lock shared_lock;
static host_submitter host_submits;

static void submit_until_stopped(void* argument) {
  host_submitter* submitter = argument;
  while (!atomic_load(&submitter->stop)) {
    host_queue_lock_take(submitter->lock);
    const VkResult result =
      submitter->queue_submit(submitter->queue, 0, NULL, VK_NULL_HANDLE);
    host_queue_lock_give(submitter->lock);
    if (result != VK_SUCCESS) {
      atomic_fetch_add(&submitter->failures, 1);
    }
    if (atomic_fetch_add(&submitter->submissions, 1) == 0) {
      mln_test_pulse();
    }
  }
}

// Resets the shared lock and returns a queue lock on it for a session.
static mln_queue_lock reset_shared_lock(void) {
  atomic_store(&shared_lock.next_ticket, 0);
  atomic_store(&shared_lock.serving, 0);
  atomic_store(&shared_lock.locks, 0);
  atomic_store(&shared_lock.unlocks, 0);
  atomic_store(&shared_lock.releases, 0);
  return (mln_queue_lock){
    .lock = session_takes_queue_lock,
    .unlock = session_gives_queue_lock,
    .user_data = &shared_lock,
    .release_user_data = queue_lock_released,
  };
}

// A host that submits on the session's queue passes a lock, and the session
// holds it around every call on the queue, its teardown drain included, so
// both can use one queue at once. The lock's release runs once the session is
// destroyed.
static void a_session_takes_the_host_queue_lock_around_its_queue_calls(void) {
  observer_reset(&first_observer);
  const mln_queue_lock queue_lock = reset_shared_lock();
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_borrowed_texture(
      map, &fixture, first_observer_wrap, NULL, &queue_lock
    ),
    mln_test_graphics_last_error()
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, fixture.driver);

  mln_test_graphics_context context = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(
    mln_test_render_fixture_graphics(&fixture), &context
  ));
  const PFN_vkGetDeviceProcAddr get_device_proc_addr =
    (PFN_vkGetDeviceProcAddr)(uintptr_t)context.vulkan_get_device_proc_addr;
  host_submits.lock = &shared_lock;
  host_submits.queue_submit = (PFN_vkQueueSubmit)get_device_proc_addr(
    (VkDevice)context.vulkan_device, "vkQueueSubmit"
  );
  host_submits.queue = (VkQueue)context.vulkan_queue;
  atomic_store(&host_submits.stop, false);
  atomic_store(&host_submits.submissions, 0);
  atomic_store(&host_submits.failures, 0);
  TEST_ASSERT_NOT_NULL(host_submits.queue_submit);
  mln_test_thread* host =
    mln_test_thread_start(submit_until_stopped, &host_submits);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&host_submits.submissions, 1));
  render_one_frame(&fixture, 1);
  render_one_frame(&fixture, 2);
  detach(&fixture);
  atomic_store(&host_submits.stop, true);
  mln_test_thread_join(host);
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&host_submits.failures));

  assert_drained_own_queue(&first_observer);
  TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&first_observer.submissions));
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&first_observer.unlocked_submissions));
  TEST_ASSERT_EQUAL_UINT(
    atomic_load(&first_observer.submissions) +
      atomic_load(&first_observer.drains),
    atomic_load(&shared_lock.locks)
  );
  TEST_ASSERT_EQUAL_UINT(
    atomic_load(&shared_lock.locks), atomic_load(&shared_lock.unlocks)
  );
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&shared_lock.releases));

  mln_test_render_fixture_destroy(&fixture);
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&shared_lock.releases));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Abandon ends the session's graphics calls at once, so it lets go of the
// host's queue lock before it returns: the lock is given back as often as it
// was taken, and its release has run. The session renders into an owned
// texture: on the Android emulator's Vulkan driver, destroying the device after
// abandoning a session that rendered into a borrowed texture crashes the
// driver.
static void abandon_releases_the_host_queue_lock(void) {
  const mln_queue_lock queue_lock = reset_shared_lock();
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_owned_texture(
      map, &fixture, &queue_lock
    ),
    mln_test_graphics_last_error()
  );
  render_one_frame(&fixture, 1);
  TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&shared_lock.locks));

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(fixture.session, &abandoned, NULL));
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&shared_lock.releases));
  TEST_ASSERT_EQUAL_UINT(
    atomic_load(&shared_lock.locks), atomic_load(&shared_lock.unlocks)
  );

  mln_test_render_fixture_destroy(&fixture);
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&shared_lock.releases));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_failed_attach_still_owns_the_session_it_published);
  RUN_TEST(a_session_drains_its_own_queue_and_never_waits_on_the_device);
  RUN_TEST(sessions_sharing_a_queue_call_their_own_device_functions);
  RUN_TEST(a_session_takes_the_host_queue_lock_around_its_queue_calls);
  RUN_TEST(abandon_releases_the_host_queue_lock);
}
