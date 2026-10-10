// What the Vulkan driver does with the host's device and queue. Vulkan is the
// one backend that checks a context the submission accepted once the driver
// has it: the other backends reject a bad descriptor at submission or cannot
// fail after it. And a session shares its device with the host, so it waits on
// its own queue and never on the whole device, and a host that shares the
// queue with it gives it a lock that it takes around each call on the queue.
// Abandon destroys every object that the session created on the device that
// it can, after it drains the session's queue, and reports how many it keeps.

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
  descriptor.extent = (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = 32,
    .height = 32,
    .scale_factor = 1.0,
  };
  descriptor.context = (mln_vulkan_context_descriptor){
    .size = sizeof(mln_vulkan_context_descriptor),
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
    mln_vulkan_owned_texture_attach(
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
    .size = sizeof(mln_queue_lock),
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

// Counts the device objects that a session creates through the device
// functions its context resolves, less the ones it destroys. Command buffers
// and descriptor sets go with their pools, which the count includes. The host
// creates its own objects through other functions, so they are not counted.
typedef enum counted_kind {
  COUNTED_MEMORY,
  COUNTED_BUFFER,
  COUNTED_IMAGE,
  COUNTED_IMAGE_VIEW,
  COUNTED_SAMPLER,
  COUNTED_SHADER_MODULE,
  COUNTED_PIPELINE_LAYOUT,
  COUNTED_PIPELINE_CACHE,
  COUNTED_PIPELINE,
  COUNTED_DESCRIPTOR_SET_LAYOUT,
  COUNTED_DESCRIPTOR_POOL,
  COUNTED_RENDER_PASS,
  COUNTED_FRAMEBUFFER,
  COUNTED_COMMAND_POOL,
  COUNTED_FENCE,
  COUNTED_SEMAPHORE,
  COUNTED_EVENT,
  COUNTED_QUERY_POOL,
  COUNTED_SWAPCHAIN,
  COUNTED_KIND_COUNT,
} counted_kind;

static const char* const counted_kind_names[COUNTED_KIND_COUNT] = {
  [COUNTED_MEMORY] = "device memory",
  [COUNTED_BUFFER] = "buffers",
  [COUNTED_IMAGE] = "images",
  [COUNTED_IMAGE_VIEW] = "image views",
  [COUNTED_SAMPLER] = "samplers",
  [COUNTED_SHADER_MODULE] = "shader modules",
  [COUNTED_PIPELINE_LAYOUT] = "pipeline layouts",
  [COUNTED_PIPELINE_CACHE] = "pipeline caches",
  [COUNTED_PIPELINE] = "pipelines",
  [COUNTED_DESCRIPTOR_SET_LAYOUT] = "descriptor set layouts",
  [COUNTED_DESCRIPTOR_POOL] = "descriptor pools",
  [COUNTED_RENDER_PASS] = "render passes",
  [COUNTED_FRAMEBUFFER] = "framebuffers",
  [COUNTED_COMMAND_POOL] = "command pools",
  [COUNTED_FENCE] = "fences",
  [COUNTED_SEMAPHORE] = "semaphores",
  [COUNTED_EVENT] = "events",
  [COUNTED_QUERY_POOL] = "query pools",
  [COUNTED_SWAPCHAIN] = "swapchains",
};

static atomic_int live_objects[COUNTED_KIND_COUNT];
static PFN_vkGetDeviceProcAddr counting_real_get_device_proc_addr;

// How teardown orders its drain, its waits, and its destroys. The counters are
// written on the session's threads, so a case asserts on them afterwards.
static atomic_bool watching_teardown;
// The fence of the latest empty submission, which drains the queue.
static _Atomic(VkFence) pending_drain_fence;
// Drains whose wait returned since the case began watching teardown.
static atomic_uint drains_waited;
// Objects destroyed while watching teardown before any drain was waited.
static atomic_uint destroys_before_drain;
// Fence waits made by a thread that held the host queue lock.
static atomic_uint waits_holding_queue_lock;

static void count_objects(counted_kind kind, int change) {
  if (
    change < 0 && atomic_load(&watching_teardown) &&
    atomic_load(&drains_waited) == 0
  ) {
    atomic_fetch_add(&destroys_before_drain, 1);
  }
  atomic_fetch_add(&live_objects[kind], change);
}

// Defines a counting create and destroy for one kind of object, whose real
// functions it resolves into `real_##create` and `real_##destroy`.
#define COUNTED_OBJECT(kind, create, destroy, Handle, Info)                    \
  static PFN_vk##create real_##create;                                         \
  static PFN_vk##destroy real_##destroy;                                       \
  static VKAPI_ATTR VkResult VKAPI_CALL counting_##create(                     \
    VkDevice device, const Info* info, const VkAllocationCallbacks* allocator, \
    Handle* out                                                                \
  ) {                                                                          \
    const VkResult result = real_##create(device, info, allocator, out);       \
    if (result == VK_SUCCESS) {                                                \
      count_objects(kind, 1);                                                  \
    }                                                                          \
    return result;                                                             \
  }                                                                            \
  static VKAPI_ATTR void VKAPI_CALL counting_##destroy(                        \
    VkDevice device, Handle object, const VkAllocationCallbacks* allocator     \
  ) {                                                                          \
    if (object != VK_NULL_HANDLE) {                                            \
      count_objects(kind, -1);                                                 \
    }                                                                          \
    real_##destroy(device, object, allocator);                                 \
  }

COUNTED_OBJECT(
  COUNTED_MEMORY, AllocateMemory, FreeMemory, VkDeviceMemory,
  VkMemoryAllocateInfo
)
COUNTED_OBJECT(
  COUNTED_BUFFER, CreateBuffer, DestroyBuffer, VkBuffer, VkBufferCreateInfo
)
COUNTED_OBJECT(
  COUNTED_IMAGE, CreateImage, DestroyImage, VkImage, VkImageCreateInfo
)
COUNTED_OBJECT(
  COUNTED_IMAGE_VIEW, CreateImageView, DestroyImageView, VkImageView,
  VkImageViewCreateInfo
)
COUNTED_OBJECT(
  COUNTED_SAMPLER, CreateSampler, DestroySampler, VkSampler, VkSamplerCreateInfo
)
COUNTED_OBJECT(
  COUNTED_SHADER_MODULE, CreateShaderModule, DestroyShaderModule,
  VkShaderModule, VkShaderModuleCreateInfo
)
COUNTED_OBJECT(
  COUNTED_PIPELINE_LAYOUT, CreatePipelineLayout, DestroyPipelineLayout,
  VkPipelineLayout, VkPipelineLayoutCreateInfo
)
COUNTED_OBJECT(
  COUNTED_PIPELINE_CACHE, CreatePipelineCache, DestroyPipelineCache,
  VkPipelineCache, VkPipelineCacheCreateInfo
)
COUNTED_OBJECT(
  COUNTED_DESCRIPTOR_SET_LAYOUT, CreateDescriptorSetLayout,
  DestroyDescriptorSetLayout, VkDescriptorSetLayout,
  VkDescriptorSetLayoutCreateInfo
)
COUNTED_OBJECT(
  COUNTED_DESCRIPTOR_POOL, CreateDescriptorPool, DestroyDescriptorPool,
  VkDescriptorPool, VkDescriptorPoolCreateInfo
)
COUNTED_OBJECT(
  COUNTED_RENDER_PASS, CreateRenderPass, DestroyRenderPass, VkRenderPass,
  VkRenderPassCreateInfo
)
COUNTED_OBJECT(
  COUNTED_FRAMEBUFFER, CreateFramebuffer, DestroyFramebuffer, VkFramebuffer,
  VkFramebufferCreateInfo
)
COUNTED_OBJECT(
  COUNTED_COMMAND_POOL, CreateCommandPool, DestroyCommandPool, VkCommandPool,
  VkCommandPoolCreateInfo
)
COUNTED_OBJECT(
  COUNTED_FENCE, CreateFence, DestroyFence, VkFence, VkFenceCreateInfo
)
COUNTED_OBJECT(
  COUNTED_SEMAPHORE, CreateSemaphore, DestroySemaphore, VkSemaphore,
  VkSemaphoreCreateInfo
)
COUNTED_OBJECT(
  COUNTED_EVENT, CreateEvent, DestroyEvent, VkEvent, VkEventCreateInfo
)
COUNTED_OBJECT(
  COUNTED_QUERY_POOL, CreateQueryPool, DestroyQueryPool, VkQueryPool,
  VkQueryPoolCreateInfo
)
COUNTED_OBJECT(
  COUNTED_SWAPCHAIN, CreateSwapchainKHR, DestroySwapchainKHR, VkSwapchainKHR,
  VkSwapchainCreateInfoKHR
)

static PFN_vkCreateGraphicsPipelines real_CreateGraphicsPipelines;
static PFN_vkCreateComputePipelines real_CreateComputePipelines;
static PFN_vkDestroyPipeline real_DestroyPipeline;

// A pipeline call creates one pipeline per create info, and leaves the handle
// of each one that it could not create null.
static void count_pipelines(uint32_t count, const VkPipeline* pipelines) {
  for (uint32_t index = 0; index < count; index += 1) {
    if (pipelines[index] != VK_NULL_HANDLE) {
      count_objects(COUNTED_PIPELINE, 1);
    }
  }
}

static VKAPI_ATTR VkResult VKAPI_CALL counting_CreateGraphicsPipelines(
  VkDevice device, VkPipelineCache cache, uint32_t count,
  const VkGraphicsPipelineCreateInfo* infos,
  const VkAllocationCallbacks* allocator, VkPipeline* pipelines
) {
  const VkResult result = real_CreateGraphicsPipelines(
    device, cache, count, infos, allocator, pipelines
  );
  count_pipelines(count, pipelines);
  return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL counting_CreateComputePipelines(
  VkDevice device, VkPipelineCache cache, uint32_t count,
  const VkComputePipelineCreateInfo* infos,
  const VkAllocationCallbacks* allocator, VkPipeline* pipelines
) {
  const VkResult result = real_CreateComputePipelines(
    device, cache, count, infos, allocator, pipelines
  );
  count_pipelines(count, pipelines);
  return result;
}

static VKAPI_ATTR void VKAPI_CALL counting_DestroyPipeline(
  VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* allocator
) {
  if (pipeline != VK_NULL_HANDLE) {
    count_objects(COUNTED_PIPELINE, -1);
  }
  real_DestroyPipeline(device, pipeline, allocator);
}

static PFN_vkQueueSubmit real_QueueSubmit;
static PFN_vkWaitForFences real_WaitForFences;

static VKAPI_ATTR VkResult VKAPI_CALL counting_QueueSubmit(
  VkQueue queue, uint32_t submit_count, const VkSubmitInfo* submits,
  VkFence fence
) {
  if (submit_count == 0 && fence != VK_NULL_HANDLE) {
    atomic_store(&pending_drain_fence, fence);
  }
  return real_QueueSubmit(queue, submit_count, submits, fence);
}

static VKAPI_ATTR VkResult VKAPI_CALL counting_WaitForFences(
  VkDevice device, uint32_t count, const VkFence* fences, VkBool32 wait_all,
  uint64_t timeout
) {
  if (holds_host_queue_lock) {
    atomic_fetch_add(&waits_holding_queue_lock, 1);
  }
  const VkResult result =
    real_WaitForFences(device, count, fences, wait_all, timeout);
  const VkFence drain_fence = atomic_load(&pending_drain_fence);
  if (
    result == VK_SUCCESS && atomic_load(&watching_teardown) && count == 1 &&
    drain_fence != VK_NULL_HANDLE && fences[0] == drain_fence
  ) {
    atomic_fetch_add(&drains_waited, 1);
  }
  return result;
}

// Routes `name` to its counting function: keeps the device's function in
// `real_##function` and returns the counting one, or null when the device has
// no such function.
#define COUNTED_FUNCTION(function)                                        \
  if (strcmp(name, "vk" #function) == 0) {                                \
    real_##function = (PFN_vk##function)real;                             \
    return real == NULL ? NULL : (PFN_vkVoidFunction)counting_##function; \
  }

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
counting_get_device_proc_addr(VkDevice device, const char* name) {
  if (strcmp(name, "vkGetDeviceProcAddr") == 0) {
    return (PFN_vkVoidFunction)counting_get_device_proc_addr;
  }
  const PFN_vkVoidFunction real =
    counting_real_get_device_proc_addr(device, name);
  COUNTED_FUNCTION(AllocateMemory)
  COUNTED_FUNCTION(FreeMemory)
  COUNTED_FUNCTION(CreateBuffer)
  COUNTED_FUNCTION(DestroyBuffer)
  COUNTED_FUNCTION(CreateImage)
  COUNTED_FUNCTION(DestroyImage)
  COUNTED_FUNCTION(CreateImageView)
  COUNTED_FUNCTION(DestroyImageView)
  COUNTED_FUNCTION(CreateSampler)
  COUNTED_FUNCTION(DestroySampler)
  COUNTED_FUNCTION(CreateShaderModule)
  COUNTED_FUNCTION(DestroyShaderModule)
  COUNTED_FUNCTION(CreatePipelineLayout)
  COUNTED_FUNCTION(DestroyPipelineLayout)
  COUNTED_FUNCTION(CreatePipelineCache)
  COUNTED_FUNCTION(DestroyPipelineCache)
  COUNTED_FUNCTION(CreateGraphicsPipelines)
  COUNTED_FUNCTION(CreateComputePipelines)
  COUNTED_FUNCTION(DestroyPipeline)
  COUNTED_FUNCTION(CreateDescriptorSetLayout)
  COUNTED_FUNCTION(DestroyDescriptorSetLayout)
  COUNTED_FUNCTION(CreateDescriptorPool)
  COUNTED_FUNCTION(DestroyDescriptorPool)
  COUNTED_FUNCTION(CreateRenderPass)
  COUNTED_FUNCTION(DestroyRenderPass)
  COUNTED_FUNCTION(CreateFramebuffer)
  COUNTED_FUNCTION(DestroyFramebuffer)
  COUNTED_FUNCTION(CreateCommandPool)
  COUNTED_FUNCTION(DestroyCommandPool)
  COUNTED_FUNCTION(CreateFence)
  COUNTED_FUNCTION(DestroyFence)
  COUNTED_FUNCTION(CreateSemaphore)
  COUNTED_FUNCTION(DestroySemaphore)
  COUNTED_FUNCTION(CreateEvent)
  COUNTED_FUNCTION(DestroyEvent)
  COUNTED_FUNCTION(CreateQueryPool)
  COUNTED_FUNCTION(DestroyQueryPool)
  COUNTED_FUNCTION(CreateSwapchainKHR)
  COUNTED_FUNCTION(DestroySwapchainKHR)
  COUNTED_FUNCTION(QueueSubmit)
  COUNTED_FUNCTION(WaitForFences)
  return real;
}

static void counter_reset(void) {
  for (size_t kind = 0; kind < COUNTED_KIND_COUNT; kind += 1) {
    atomic_store(&live_objects[kind], 0);
  }
  atomic_store(&watching_teardown, false);
  atomic_store(&pending_drain_fence, VK_NULL_HANDLE);
  atomic_store(&drains_waited, 0);
  atomic_store(&destroys_before_drain, 0);
  atomic_store(&waits_holding_queue_lock, 0);
}

static void* counting_wrap(void* get_device_proc_addr) {
  counting_real_get_device_proc_addr =
    (PFN_vkGetDeviceProcAddr)(uintptr_t)get_device_proc_addr;
  return (void*)(uintptr_t)counting_get_device_proc_addr;
}

static int live_object_total(void) {
  int total = 0;
  for (size_t kind = 0; kind < COUNTED_KIND_COUNT; kind += 1) {
    total += atomic_load(&live_objects[kind]);
  }
  return total;
}

// Asserts that every device object the session created is destroyed, so the
// host may destroy its device.
static void assert_no_live_objects(void) {
  for (size_t kind = 0; kind < COUNTED_KIND_COUNT; kind += 1) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      0, atomic_load(&live_objects[kind]), counted_kind_names[kind]
    );
  }
}

// Abandons a session that can destroy everything it holds, and asserts that it
// did: the host may then destroy its device.
static void abandon_destroying_everything(mln_render_session session) {
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(session, &abandoned, NULL));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_CLEAN, abandoned.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(0, abandoned.quarantined_resource_count);
  assert_no_live_objects();
}

// Abandon destroys a session's device objects, so the host may destroy its
// device once abandon returns. It drains the queue before it destroys any of
// them, and holds the host's queue lock only around the drain's submission,
// never across a wait. It then releases the lock: the lock is given back as
// often as it was taken, and its release has run.
static void abandon_drains_under_the_host_queue_lock_and_releases_it(void) {
  counter_reset();
  const mln_queue_lock queue_lock = reset_shared_lock();
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_borrowed_texture(
      map, &fixture, counting_wrap, NULL, &queue_lock
    ),
    mln_test_graphics_last_error()
  );
  render_one_frame(&fixture, 1);
  TEST_ASSERT_GREATER_THAN_INT(0, live_object_total());
  const unsigned int locks_before = atomic_load(&shared_lock.locks);
  TEST_ASSERT_GREATER_THAN_UINT(0, locks_before);
  // The barrier orders the rest of the frame's driver call before the watch.
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  atomic_store(&watching_teardown, true);

  abandon_destroying_everything(fixture.session);
  TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&drains_waited));
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&destroys_before_drain));
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&waits_holding_queue_lock));
  TEST_ASSERT_GREATER_THAN_UINT(locks_before, atomic_load(&shared_lock.locks));
  TEST_ASSERT_EQUAL_UINT(
    atomic_load(&shared_lock.locks), atomic_load(&shared_lock.unlocks)
  );
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&shared_lock.releases));

  mln_test_render_fixture_destroy(&fixture);
  TEST_ASSERT_EQUAL_UINT(1, atomic_load(&shared_lock.releases));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A session-owned ring that the host holds no frame of belongs to the session
// alone, so abandon destroys it with the rest. That includes a ring whose
// frame the host released just before abandon, whether or not the driver has
// run the release yet: CPU_COMPLETE attests that the host's GPU read is done.
static void abandon_destroys_an_owned_texture_ring(void) {
  counter_reset();
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_owned_texture(
      map, &fixture, counting_wrap, NULL
    ),
    mln_test_graphics_last_error()
  );
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);
  TEST_ASSERT_GREATER_THAN_INT(0, atomic_load(&live_objects[COUNTED_IMAGE]));
  MLN_TEST_OK(mln_acquired_frame_release(&frame, NULL, NULL));

  abandon_destroying_everything(fixture.session);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A swapchain is a child of both the device and the host's surface, so abandon
// destroys it before the host may destroy either.
static void abandon_destroys_a_surface_swapchain(void) {
  counter_reset();
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_surface(map, &fixture, counting_wrap),
    mln_test_graphics_last_error()
  );
  render_one_frame(&fixture, 1);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&live_objects[COUNTED_SWAPCHAIN]));

  abandon_destroying_everything(fixture.session);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_failed_attach_still_owns_the_session_it_published);
  RUN_TEST(a_session_drains_its_own_queue_and_never_waits_on_the_device);
  RUN_TEST(sessions_sharing_a_queue_call_their_own_device_functions);
  RUN_TEST(a_session_takes_the_host_queue_lock_around_its_queue_calls);
  RUN_TEST(abandon_drains_under_the_host_queue_lock_and_releases_it);
  RUN_TEST(abandon_destroys_an_owned_texture_ring);
  RUN_TEST(abandon_destroys_a_surface_swapchain);
}
