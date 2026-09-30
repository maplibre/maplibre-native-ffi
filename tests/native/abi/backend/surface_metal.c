// A Metal surface session retargeted to a new layer configures that layer and
// presents into it.

#include <objc/message.h>
#include <objc/runtime.h>
#include <stdint.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

extern void* MTLCreateSystemDefaultDevice(void);

// CGSize, which is two doubles on every 64-bit Apple target.
typedef struct layer_size {
  double width;
  double height;
} layer_size;

static id new_object(const char* class_name) {
  Class class_object = objc_getClass(class_name);
  if (class_object == Nil) {
    return nil;
  }
  return ((id (*)(Class, SEL))objc_msgSend)(
    class_object, sel_registerName("new")
  );
}

static void release_object(void* object) {
  if (object != NULL) {
    ((void (*)(id, SEL))objc_msgSend)((id)object, sel_registerName("release"));
  }
}

static layer_size drawable_size(id layer) {
  return ((layer_size (*)(id, SEL))objc_msgSend)(
    layer, sel_registerName("drawableSize")
  );
}

// Demands one presented frame and returns its disposition.
static uint32_t present_frame(mln_render_session session) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_PRESENT;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(session, &demand, NULL)
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_render_session_barrier(session, &completion.descriptor, NULL)
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_drain_frame_results(session, &batch, NULL)
  );
  size_t count = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_frame_batch_count(batch, &count, NULL)
  );
  TEST_ASSERT_EQUAL_size_t(1, count);
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_frame_batch_get(batch, 0, &result, NULL)
  );
  mln_render_frame_batch_release(batch);
  return result.disposition;
}

static void metal_surface_retarget_presents_into_the_new_layer(void) {
  void* device = MTLCreateSystemDefaultDevice();
  id initial_layer = new_object("CAMetalLayer");
  id replacement_layer = new_object("CAMetalLayer");
  TEST_ASSERT_NOT_NULL_MESSAGE(device, "this host has no Metal device");
  TEST_ASSERT_NOT_NULL(initial_layer);
  TEST_ASSERT_NOT_NULL(replacement_layer);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(map, mln_test_red_background_style_json)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));

  mln_metal_surface_descriptor descriptor =
    mln_metal_surface_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context.device = device;
  descriptor.layer = initial_layer;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  mln_render_session session = MLN_HANDLE_NULL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_metal_surface_attach(
      map, &descriptor, &options, &session, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, present_frame(session));

  descriptor.layer = replacement_layer;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_metal_surface_set_target(
                     session, &descriptor, &completion.descriptor, NULL
                   )
  );
  const layer_size size = drawable_size(replacement_layer);
  TEST_ASSERT_EQUAL_INT(64, (int)size.width);
  TEST_ASSERT_EQUAL_INT(64, (int)size.height);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, present_frame(session));

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_render_session_detach(session, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_destroy(session, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  release_object(replacement_layer);
  release_object(initial_layer);
  release_object(device);
}

MLN_TEST_GROUP { RUN_TEST(metal_surface_retarget_presents_into_the_new_layer); }
