// The depth that a Vulkan ring's frames keep in the host's depth images.

#include <math.h>
#include <vulkan/vulkan_core.h>

#include "mln_test_graphics.h"
#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/style.h"
#include "support/test_support.h"

enum { side = MLN_TEST_HOST_TARGET_SIZE };

// The extrusion's height, in meters.
static const double height = 30.0;

// An opaque fill under a fill extrusion 30 meters high, both wider than the
// view, which looks straight down on the extrusion's roof.
static const char block_style[] =
  "{\"version\":8,\"sources\":{\"block\":{\"type\":\"geojson\","
  "\"data\":{\"type\":\"Polygon\",\"coordinates\":[[[-1,-1],[1,-1],[1,1],"
  "[-1,1],[-1,-1]]]}}},\"layers\":[{\"id\":\"ground\",\"type\":\"fill\","
  "\"source\":\"block\",\"paint\":{\"fill-color\":\"#00ff00\"}},"
  "{\"id\":\"block\",\"type\":\"fill-extrusion\",\"source\":\"block\","
  "\"paint\":{\"fill-extrusion-color\":\"#ff0000\","
  "\"fill-extrusion-height\":30}}],"
  "\"transition\":{\"duration\":0,\"delay\":0}}";

// The depth that `projection` gives the point of the camera's center at
// `altitude` meters.
static double depth_at_center(
  const mln_frame_projection* projection, mln_lat_lng center, double altitude
) {
  const double pi = 3.14159265358979323846;
  const double latitude = center.latitude * pi / 180.0;
  const double world[4] = {
    (center.longitude + 180.0) / 360.0,
    (1.0 - log(tan(pi / 4.0 + latitude / 2.0)) / pi) / 2.0, altitude, 1.0
  };
  const mln_mat4* matrix = &projection->matrix;
  const mln_vec4* columns[4] = {&matrix->x, &matrix->y, &matrix->z, &matrix->w};
  double clip[4] = {0};
  for (int column = 0; column < 4; column += 1) {
    clip[0] += columns[column]->x * world[column];
    clip[1] += columns[column]->y * world[column];
    clip[2] += columns[column]->z * world[column];
    clip[3] += columns[column]->w * world[column];
  }
  return clip[2] / clip[3];
}

// Each frame keeps its depth and stencil in the host's depth image of its
// slot, and the acquired frame names that image. Looking straight down, the
// roof has one depth, which the frame's projection gives its points.
static void a_rings_depth_images_hold_the_frames_depth(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(block_style));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_depth_ring(map, &fixture, 2),
    mln_test_graphics_last_error()
  );
  const mln_lat_lng center = {.latitude = 0.1, .longitude = 0.2};
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                         MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_ROLL |
                         MLN_CAMERA_OPTION_PADDING;
  update.camera.center = center;
  update.camera.zoom = 16.0;
  update.camera.bearing = 20.0;
  update.camera.roll = 10.0;
  update.camera.padding = (mln_edge_insets){.top = 10.0, .left = 6.0};
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  mln_test_render_until_idle(runtime, map, &fixture);

  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);
  mln_render_frame_result result = {.size = sizeof(result)};
  MLN_TEST_OK(mln_acquired_frame_get_result(frame, &result, NULL));
  mln_vulkan_texture_frame texture = {.size = sizeof(texture)};
  MLN_TEST_OK(mln_acquired_frame_get_vulkan_texture(frame, &texture, NULL));
  mln_test_graphics_texture* depth =
    mln_test_render_fixture_depth_texture(&fixture, texture.slot);
  TEST_ASSERT_NOT_NULL(depth);
  mln_test_graphics_texture_info info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_texture_get_info(depth, &info));
  TEST_ASSERT_EQUAL_UINT64(info.vulkan_image, texture.depth_image);
  TEST_ASSERT_EQUAL_UINT64(info.vulkan_image_view, texture.depth_image_view);
  TEST_ASSERT_EQUAL_UINT32(info.format, texture.depth_format);
  TEST_ASSERT_EQUAL_UINT32(info.vulkan_final_layout, texture.depth_layout);
  mln_test_render_release_frame(&frame);
  // The readback needs the session idle.
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );

  static float depths[side * side];
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_graphics_depth_texture_read(depth, depths, side * side),
    mln_test_graphics_last_error()
  );
  const mln_frame_projection* projection = &result.frame_projection;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_CLIP_DEPTH_RANGE_ZERO_TO_ONE, projection->clip_depth_range
  );
  const double roof = depth_at_center(projection, center, height);
  TEST_ASSERT_TRUE(roof > 0.0 && roof < projection->far_depth);
  for (size_t index = 0; index < side * side; index += 1) {
    TEST_ASSERT_DOUBLE_WITHIN(1e-4, roof, depths[index]);
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A ring takes depth images only one for each of its images, and only in the
// device's formats.
static void an_attach_rejects_depth_that_the_ring_cannot_take(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_graphics* graphics =
    mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_VULKAN);
  TEST_ASSERT_NOT_NULL_MESSAGE(graphics, mln_test_graphics_last_error());
  mln_test_graphics_context context = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(graphics, &context));
  mln_test_graphics_texture* color =
    mln_test_graphics_texture_create(graphics, side, side);
  mln_test_graphics_texture* other_color =
    mln_test_graphics_texture_create(graphics, side, side);
  mln_test_graphics_texture* depth =
    mln_test_graphics_depth_texture_create(graphics, side, side);
  TEST_ASSERT_NOT_NULL(color);
  TEST_ASSERT_NOT_NULL(other_color);
  TEST_ASSERT_NOT_NULL(depth);
  mln_test_graphics_texture_info color_info = {0};
  mln_test_graphics_texture_info other_info = {0};
  mln_test_graphics_texture_info depth_info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_texture_get_info(color, &color_info));
  TEST_ASSERT_TRUE(
    mln_test_graphics_texture_get_info(other_color, &other_info)
  );
  TEST_ASSERT_TRUE(mln_test_graphics_texture_get_info(depth, &depth_info));
  const mln_vulkan_device_descriptor descriptor = {
    .size = sizeof(mln_vulkan_device_descriptor),
    .context = {
      .instance = context.vulkan_instance,
      .physical_device = context.vulkan_physical_device,
      .device = context.vulkan_device,
      .graphics_queue = context.vulkan_queue,
      .graphics_queue_family_index = context.vulkan_queue_family_index,
      .get_instance_proc_addr = context.vulkan_get_instance_proc_addr,
      .get_device_proc_addr = context.vulkan_get_device_proc_addr,
    },
  };
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_wrap_vulkan(&descriptor, &device, NULL));
  mln_gpu_device_capabilities capabilities = {.size = sizeof(capabilities)};
  MLN_TEST_OK(mln_gpu_device_get_capabilities(device, &capabilities, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT32(
    0, capabilities.depth_stencil_formats &
         MLN_DEPTH_STENCIL_FORMAT_FLAG_DEPTH32_FLOAT_STENCIL8
  );

  const mln_vulkan_borrowed_texture colors[2] = {
    {.image = color_info.vulkan_image,
     .image_view = color_info.vulkan_image_view},
    {.image = other_info.vulkan_image,
     .image_view = other_info.vulkan_image_view},
  };
  const mln_vulkan_borrowed_texture depths[1] = {
    {.image = depth_info.vulkan_image,
     .image_view = depth_info.vulkan_image_view},
  };
  mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_VULKAN_BORROWED_TEXTURES,
  };
  target.data.vulkan_borrowed_textures = (mln_vulkan_borrowed_textures_target){
    .logical_size = {.width = side, .height = side},
    .physical_width = side,
    .physical_height = side,
    .textures = colors,
    .texture_count = 1,
    .format = color_info.format,
    .final_layout = color_info.vulkan_final_layout,
    .depth_textures = depths,
    .depth_texture_count = 1,
    .depth_format = depth_info.format,
    .depth_final_layout = depth_info.vulkan_final_layout,
  };
  const mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
  };
  const mln_completion discard = mln_test_discard_completion();
  mln_render_session session = MLN_HANDLE_NULL;
  // Two images, but one depth image.
  target.data.vulkan_borrowed_textures.texture_count = 2;
  TEST_ASSERT_EQUAL_INT32(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_attach(map, &target, device, &options, &session, &discard, NULL)
  );
  // A depth format that is no depth and stencil format.
  target.data.vulkan_borrowed_textures.texture_count = 1;
  target.data.vulkan_borrowed_textures.depth_format = VK_FORMAT_D16_UNORM;
  TEST_ASSERT_EQUAL_INT32(
    MLN_STATUS_UNSUPPORTED,
    mln_map_attach(map, &target, device, &options, &session, &discard, NULL)
  );
  // A borrowed ring keeps depth in the host's images only.
  target.data.vulkan_borrowed_textures.depth_format = depth_info.format;
  mln_render_session_attach_options keeping = options;
  keeping.keep_depth = true;
  TEST_ASSERT_EQUAL_INT32(
    MLN_STATUS_UNSUPPORTED,
    mln_map_attach(map, &target, device, &keeping, &session, &discard, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);

  mln_gpu_device_release(device);
  mln_test_graphics_texture_destroy(depth);
  mln_test_graphics_texture_destroy(other_color);
  mln_test_graphics_texture_destroy(color);
  mln_test_graphics_destroy(graphics);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_rings_depth_images_hold_the_frames_depth);
  RUN_TEST(an_attach_rejects_depth_that_the_ring_cannot_take);
}
