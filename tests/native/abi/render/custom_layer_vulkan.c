// A custom layer that draws with Vulkan commands in the map's render pass, at
// its place among the map's layers.

#include <stdatomic.h>
#include <vulkan/vulkan_core.h>

#include "support/frames.h"
#include "support/style.h"
#include "support/test_support.h"

enum { side = 64, pixel_bytes = side * side * 4 };

// Clears the middle of the color attachment to green from inside the pass, and
// records what the callbacks got.
typedef struct clearing_host {
  atomic_int prerenders;
  atomic_int renders;
  mln_render_backend prerender_backend;
  void* prerender_command_buffer;
  mln_vulkan_custom_layer_pass pass;
} clearing_host;

static void on_prerender(
  void* user_data, const mln_custom_layer_prerender_parameters* parameters
) {
  clearing_host* host = user_data;
  host->prerender_backend = parameters->backend;
  if (parameters->backend == MLN_RENDER_BACKEND_VULKAN) {
    host->prerender_command_buffer = parameters->encoder.vulkan.command_buffer;
  }
  atomic_fetch_add(&host->prerenders, 1);
}

static void on_render(
  void* user_data, const mln_custom_layer_render_parameters* parameters
) {
  clearing_host* host = user_data;
  if (parameters->backend != MLN_RENDER_BACKEND_VULKAN) return;
  const mln_vulkan_custom_layer_pass* pass = &parameters->pass.vulkan;
  host->pass = *pass;
  PFN_vkGetDeviceProcAddr get_device_proc_addr =
    (PFN_vkGetDeviceProcAddr)pass->context.get_device_proc_addr;
  PFN_vkCmdClearAttachments clear_attachments =
    (PFN_vkCmdClearAttachments)get_device_proc_addr(
      (VkDevice)pass->context.device, "vkCmdClearAttachments"
    );
  const VkClearAttachment attachment = {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .colorAttachment = 0,
    .clearValue = {.color = {.float32 = {0.0f, 1.0f, 0.0f, 1.0f}}},
  };
  const VkClearRect rect = {
    .rect =
      {
        .offset =
          {(int32_t)parameters->frame.width / 4,
           (int32_t)parameters->frame.height / 4},
        .extent = {parameters->frame.width / 2, parameters->frame.height / 2},
      },
    .baseArrayLayer = 0,
    .layerCount = 1,
  };
  clear_attachments(
    (VkCommandBuffer)pass->command_buffer, 1, &attachment, 1, &rect
  );
  atomic_fetch_add(&host->renders, 1);
}

static void add_layer_json(mln_map map, const char* json) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_buffer_view_from_cstr(json), (mln_buffer_view){0},
    &completion.descriptor, NULL
  ));
}

static void add_clearing_layer(
  mln_map map, const char* before, clearing_host* host
) {
  const mln_custom_layer_host registration = {
    .size = sizeof(mln_custom_layer_host),
    .prerender = on_prerender,
    .render = on_render,
    .user_data = host,
  };
  MLN_TEST_AWAIT_OK(mln_map_add_custom_layer(
    map, mln_buffer_view_from_cstr("clear"),
    before == NULL ? (mln_buffer_view){0} : mln_buffer_view_from_cstr(before),
    &registration, &completion.descriptor, NULL
  ));
}

// The pixel at column `x` and row `y` of a readback.
static const uint8_t* pixel(const uint8_t* pixels, int x, int y) {
  return &pixels[(y * side + x) * 4];
}

static void expect_pixel(
  const uint8_t* pixels, int x, int y, int red, int green, int blue
) {
  const uint8_t* actual = pixel(pixels, x, y);
  TEST_ASSERT_UINT8_WITHIN(3, red, actual[0]);
  TEST_ASSERT_UINT8_WITHIN(3, green, actual[1]);
  TEST_ASSERT_UINT8_WITHIN(3, blue, actual[2]);
  TEST_ASSERT_EQUAL_UINT8(255, actual[3]);
}

// Renders a frame over a red background and under a half-transparent blue
// layer, with the custom layer below the layer named `before`, and reads it
// back.
static void render_between_layers(
  const char* before, clearing_host* host, uint8_t* pixels
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  add_layer_json(
    map,
    "{\"id\":\"red\",\"type\":\"background\","
    "\"paint\":{\"background-color\":\"#ff0000\"}}"
  );
  add_layer_json(
    map,
    "{\"id\":\"blue\",\"type\":\"background\","
    "\"paint\":{\"background-color\":\"#0000ff\",\"background-opacity\":0.5}}"
  );
  add_clearing_layer(map, before, host);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(&fixture)
  );
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(&fixture, &info, pixels, pixel_bytes));
  TEST_ASSERT_EQUAL_UINT32(side, info.width);
  TEST_ASSERT_EQUAL_UINT32(side, info.height);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_layer_below_another_draws_under_it(void) {
  static uint8_t pixels[pixel_bytes];
  clearing_host host = {0};
  render_between_layers("blue", &host, pixels);

  TEST_ASSERT_EQUAL_INT(1, atomic_load(&host.renders));
  expect_pixel(pixels, side / 2, side / 2, 0, 128, 128);
  expect_pixel(pixels, 2, 2, 128, 0, 128);
}

static void a_layer_above_every_layer_draws_over_them(void) {
  static uint8_t pixels[pixel_bytes];
  clearing_host host = {0};
  render_between_layers(NULL, &host, pixels);

  TEST_ASSERT_EQUAL_INT(1, atomic_load(&host.renders));
  expect_pixel(pixels, side / 2, side / 2, 0, 255, 0);
  expect_pixel(pixels, 2, 2, 128, 0, 128);
}

// The callbacks get the frame's command buffers, outside the pass and inside
// it, and the pass's render pass and formats.
static void the_callbacks_get_the_frame_s_vulkan_commands(void) {
  static uint8_t pixels[pixel_bytes];
  clearing_host host = {0};
  render_between_layers(NULL, &host, pixels);

  TEST_ASSERT_EQUAL_INT(1, atomic_load(&host.prerenders));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_BACKEND_VULKAN, host.prerender_backend);
  TEST_ASSERT_NOT_NULL(host.prerender_command_buffer);
  TEST_ASSERT_NOT_EQUAL_UINT64(
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL, host.pass.render_pass
  );
  TEST_ASSERT_EQUAL_UINT32(0, host.pass.subpass);
  TEST_ASSERT_EQUAL_UINT32(VK_SAMPLE_COUNT_1_BIT, host.pass.sample_count);
  TEST_ASSERT_NOT_EQUAL_UINT32(VK_FORMAT_UNDEFINED, host.pass.color_format);
  TEST_ASSERT_NOT_NULL(host.pass.context.device);
  TEST_ASSERT_NOT_NULL(host.pass.context.get_device_proc_addr);
}

MLN_TEST_GROUP {
  RUN_TEST(a_layer_below_another_draws_under_it);
  RUN_TEST(a_layer_above_every_layer_draws_over_them);
  RUN_TEST(the_callbacks_get_the_frame_s_vulkan_commands);
}
