// A borrowed ring of WebGPU textures that the host created on its device. The
// session's writes to a frame's texture are on the device's queue when the
// frame becomes acquirable, so the host reads the texture through that queue.

#include <string.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { pixel_count = 64 * 64 };

static const uint8_t red[4] = {255, 0, 0, 255};

// An acquired frame of a borrowed ring needs no producer synchronization
// object. A copy that the host submits to the device's queue right after
// acquisition, with no fence of its own, reads the frame the session rendered.
static void a_borrowed_ring_frame_is_ordered_before_the_hosts_reads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_red_background_style_json
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_webgpu_borrowed_ring_create(map, &fixture, 2));

  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);
  mln_gpu_sync producer = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(frame, &producer, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_CPU_COMPLETE, producer.kind);
  mln_webgpu_texture_frame record = {.size = sizeof(record)};
  MLN_TEST_OK(mln_acquired_frame_get_webgpu_texture(frame, &record, NULL));
  TEST_ASSERT_EQUAL_PTR(
    mln_test_webgpu_ring_texture(&fixture, record.slot), record.texture
  );

  static uint8_t pixels[pixel_count * 4];
  TEST_ASSERT_TRUE(mln_test_webgpu_read_ring_texture(
    &fixture, record.slot, pixels, sizeof(pixels)
  ));
  size_t matching = 0;
  for (size_t index = 0; index < pixel_count; index += 1) {
    matching += memcmp(&pixels[index * 4], red, 4) == 0;
  }
  TEST_ASSERT_EQUAL_size_t(pixel_count, matching);

  mln_test_render_release_frame(&frame);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_borrowed_ring_frame_is_ordered_before_the_hosts_reads);
}
