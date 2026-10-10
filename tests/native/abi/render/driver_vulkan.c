// What the Vulkan driver does with the host's device and queue. Vulkan is the
// one backend that checks a context the submission accepted once the driver
// has it: the other backends reject a bad descriptor at submission or cannot
// fail after it. And a session shares its device with the host, so it waits on
// its own queue and never on the whole device.

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

// The device functions a session resolves through observing_proc_addr(). It
// counts device waits and remembers the fence of the latest submission. A
// fence signals only once everything submitted before it has completed, and
// may be destroyed only after that, so the session is done with every image
// once that fence has signaled or been destroyed. The case reads this state
// only after a completion orders it after the driver's writes.
static PFN_vkGetDeviceProcAddr real_get_device_proc_addr;
static PFN_vkDeviceWaitIdle real_device_wait_idle;
static PFN_vkQueueSubmit real_queue_submit;
static PFN_vkDestroyFence real_destroy_fence;
static PFN_vkGetFenceStatus real_get_fence_status;
static VkDevice observed_device;
static atomic_uint device_wait_count;
static atomic_uint submission_count;
static _Atomic(VkFence) latest_fence;
static atomic_bool latest_fence_destroyed;

static VKAPI_ATTR VkResult VKAPI_CALL
observed_device_wait_idle(VkDevice device) {
  atomic_fetch_add(&device_wait_count, 1);
  return real_device_wait_idle(device);
}

static VKAPI_ATTR VkResult VKAPI_CALL observed_queue_submit(
  VkQueue queue, uint32_t submit_count, const VkSubmitInfo* submits,
  VkFence fence
) {
  atomic_fetch_add(&submission_count, 1);
  atomic_store(&latest_fence, fence);
  atomic_store(&latest_fence_destroyed, false);
  return real_queue_submit(queue, submit_count, submits, fence);
}

static VKAPI_ATTR void VKAPI_CALL observed_destroy_fence(
  VkDevice device, VkFence fence, const VkAllocationCallbacks* allocator
) {
  if (fence != VK_NULL_HANDLE && fence == atomic_load(&latest_fence)) {
    atomic_store(&latest_fence_destroyed, true);
  }
  real_destroy_fence(device, fence, allocator);
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
observed_get_device_proc_addr(VkDevice device, const char* name) {
  // Like a layer, it answers for itself, because a dispatch table looks
  // vkGetDeviceProcAddr up through the one it was given.
  if (strcmp(name, "vkGetDeviceProcAddr") == 0) {
    return (PFN_vkVoidFunction)observed_get_device_proc_addr;
  }
  const PFN_vkVoidFunction real = real_get_device_proc_addr(device, name);
  if (strcmp(name, "vkDeviceWaitIdle") == 0) {
    real_device_wait_idle = (PFN_vkDeviceWaitIdle)real;
    return (PFN_vkVoidFunction)observed_device_wait_idle;
  }
  if (strcmp(name, "vkQueueSubmit") == 0) {
    real_queue_submit = (PFN_vkQueueSubmit)real;
    return (PFN_vkVoidFunction)observed_queue_submit;
  }
  if (strcmp(name, "vkDestroyFence") == 0) {
    observed_device = device;
    real_destroy_fence = (PFN_vkDestroyFence)real;
    real_get_fence_status = (PFN_vkGetFenceStatus)real_get_device_proc_addr(
      device, "vkGetFenceStatus"
    );
    return (PFN_vkVoidFunction)observed_destroy_fence;
  }
  return real;
}

static void* observing_proc_addr(void* get_device_proc_addr) {
  real_get_device_proc_addr =
    (PFN_vkGetDeviceProcAddr)(uintptr_t)get_device_proc_addr;
  return (void*)(uintptr_t)observed_get_device_proc_addr;
}

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

// A host compositor may submit on its own queue while a session renders, so a
// session never waits on the whole device, which would need every queue. A
// texture replacement still leaves the outgoing image free once it completes,
// because the session has waited for all of its own work by then.
static void a_session_waits_on_its_own_work_and_never_on_the_device(void) {
  atomic_store(&device_wait_count, 0);
  atomic_store(&submission_count, 0);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_borrowed_texture(
      map, &fixture, observing_proc_addr
    ),
    mln_test_graphics_last_error()
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, fixture.driver);
  render_one_frame(&fixture, 1);

  mln_test_graphics_texture* replacement =
    mln_test_render_fixture_new_texture(&fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_test_render_fixture_set_texture(
      &fixture, mln_test_render_fixture_graphics(&fixture), replacement,
      &completion.descriptor
    )
  );
  // The latest submission covers every earlier one on the queue.
  TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&submission_count));
  const VkFence fence = atomic_load(&latest_fence);
  TEST_ASSERT_TRUE(fence != VK_NULL_HANDLE);
  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&latest_fence_destroyed) ||
      real_get_fence_status(observed_device, fence) == VK_SUCCESS,
    "the session's work on the replaced texture is still pending"
  );

  render_one_frame(&fixture, 2);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_UINT(0, atomic_load(&device_wait_count));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_failed_attach_still_owns_the_session_it_published);
  RUN_TEST(a_session_waits_on_its_own_work_and_never_on_the_device);
}
