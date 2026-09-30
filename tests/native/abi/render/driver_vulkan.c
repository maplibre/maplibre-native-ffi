// A Vulkan attach that fails on the driver. Vulkan is the one backend that
// checks a context the submission accepted once the driver has it: the other
// backends reject a bad descriptor at submission or cannot fail after it.

#include <stdint.h>

#include "mln_test_graphics.h"
#include "support/frames.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

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
  mln_test_completion attach = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_vulkan_owned_texture_attach(
      map, &descriptor, &options, &session, &attach.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_finish(&attach)
  );
  mln_test_completion_destroy(&attach);

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(session, &snapshot, NULL)
  );
  TEST_ASSERT_NOT_EQUAL_UINT32(
    MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_render_session_destroy(session, NULL)
  );
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_abandon(session, &abandoned, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_destroy(session, NULL)
  );

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_graphics_destroy(graphics);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_failed_attach_still_owns_the_session_it_published);
}
