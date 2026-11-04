// Vulkan devices that the library creates, and the sessions that attach with
// their context. Every build validates the options before it reports that it
// has no Vulkan backend.

#include <string.h>

#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
#include <vulkan/vulkan_core.h>
#endif

static void create_rejects_malformed_options(void) {
  static const uint8_t short_id[7] = {0};
  mln_vulkan_device device = MLN_HANDLE_NULL;
  mln_vulkan_device_options options = {
    .size = sizeof(mln_vulkan_device_options) - 1
  };
  MLN_TEST_INVALID(
    mln_vulkan_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  options.size = sizeof(mln_vulkan_device_options);
  options.fields = 1U << 31U;
  MLN_TEST_INVALID(
    mln_vulkan_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  options.fields = MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID;
  options.device_uuid =
    (mln_buffer_view){.data = short_id, .size = sizeof(short_id)};
  MLN_TEST_INVALID(
    mln_vulkan_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  options.fields = MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID;
  options.device_luid =
    (mln_buffer_view){.data = short_id, .size = sizeof(short_id)};
  MLN_TEST_INVALID(
    mln_vulkan_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  MLN_TEST_INVALID(mln_vulkan_device_create(NULL, NULL, MLN_TEST_DIAGNOSTIC));
  device = UINT64_C(1);
  MLN_TEST_INVALID(
    mln_vulkan_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC)
  );
  mln_vulkan_device_context_result result = {.size = sizeof(result)};
  MLN_TEST_INVALID(
    mln_vulkan_device_get_context(MLN_HANDLE_NULL, &result, MLN_TEST_DIAGNOSTIC)
  );
  mln_vulkan_device_release(MLN_HANDLE_NULL);
}

#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
static void create_reports_an_unsupported_backend(void) {
  mln_vulkan_device device = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_vulkan_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}
#else

enum { pixel_count = MLN_TEST_HOST_TARGET_SIZE * MLN_TEST_HOST_TARGET_SIZE };

static mln_vulkan_device create_device(
  const mln_vulkan_device_options* options
) {
  mln_vulkan_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_vulkan_device_create(options, &device, MLN_TEST_DIAGNOSTIC));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
  return device;
}

static mln_vulkan_context_descriptor device_context(mln_vulkan_device device) {
  mln_vulkan_device_context_result result = {.size = sizeof(result)};
  MLN_TEST_OK(
    mln_vulkan_device_get_context(device, &result, MLN_TEST_DIAGNOSTIC)
  );
  return result.context;
}

static PFN_vkVoidFunction instance_function(
  const mln_vulkan_context_descriptor* context, const char* name
) {
  const PFN_vkGetInstanceProcAddr get_instance_proc_addr =
    (PFN_vkGetInstanceProcAddr)(uintptr_t)context->get_instance_proc_addr;
  return get_instance_proc_addr((VkInstance)context->instance, name);
}

// The deviceUUID of `physical_device` on the context's instance.
static void physical_device_uuid(
  const mln_vulkan_context_descriptor* context, void* physical_device,
  uint8_t out_uuid[VK_UUID_SIZE]
) {
  const PFN_vkGetPhysicalDeviceProperties2 get_properties =
    (PFN_vkGetPhysicalDeviceProperties2)instance_function(
      context, "vkGetPhysicalDeviceProperties2"
    );
  TEST_ASSERT_NOT_NULL(get_properties);
  VkPhysicalDeviceIDProperties ids = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
  };
  VkPhysicalDeviceProperties2 properties = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    .pNext = &ids,
  };
  get_properties((VkPhysicalDevice)physical_device, &properties);
  memcpy(out_uuid, ids.deviceUUID, VK_UUID_SIZE);
}

static mln_test_graphics* borrow_graphics(
  const mln_vulkan_context_descriptor* context
) {
  const mln_test_graphics_context borrowed = {
    .backend = MLN_TEST_GRAPHICS_BACKEND_VULKAN,
    .vulkan_queue_family_index = context->graphics_queue_family_index,
    .vulkan_instance = context->instance,
    .vulkan_physical_device = context->physical_device,
    .vulkan_device = context->device,
    .vulkan_queue = context->graphics_queue,
    .vulkan_get_instance_proc_addr = context->get_instance_proc_addr,
    .vulkan_get_device_proc_addr = context->get_device_proc_addr,
  };
  mln_test_graphics* graphics =
    mln_test_graphics_create_vulkan_borrowed(&borrowed);
  TEST_ASSERT_NOT_NULL_MESSAGE(graphics, mln_test_graphics_last_error());
  return graphics;
}

static void render_one_frame(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_demand_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
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

static void a_session_renders_with_the_device_context(void) {
  const mln_vulkan_device device = create_device(NULL);
  const mln_vulkan_context_descriptor context = device_context(device);
  TEST_ASSERT_NOT_NULL(context.instance);
  TEST_ASSERT_NOT_NULL(context.physical_device);
  TEST_ASSERT_NOT_NULL(context.device);
  TEST_ASSERT_NOT_NULL(context.graphics_queue);
  mln_test_graphics* graphics = borrow_graphics(&context);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_on(map, &fixture, graphics),
    mln_test_graphics_last_error()
  );

  render_one_frame(&fixture, 1);
  detach(&fixture);
  static uint8_t pixels[pixel_count * 4];
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_read_texture(&fixture, pixels, sizeof(pixels)),
    mln_test_graphics_last_error()
  );
  static const uint8_t red[4] = {255, 0, 0, 255};
  size_t matching = 0;
  for (size_t index = 0; index < pixel_count; index += 1) {
    matching += memcmp(&pixels[index * 4], red, 4) == 0;
  }
  TEST_ASSERT_EQUAL_size_t(pixel_count, matching);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_graphics_destroy(graphics);
  mln_vulkan_device_release(device);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

enum { max_gpus = 8 };

// The GPUs of the context's instance, with their UUIDs and whether each has a
// graphics queue.
typedef struct gpu_list {
  uint32_t count;
  VkPhysicalDevice gpus[max_gpus];
  uint8_t uuids[max_gpus][VK_UUID_SIZE];
  bool graphics[max_gpus];
} gpu_list;

static gpu_list list_gpus(const mln_vulkan_context_descriptor* context) {
  const PFN_vkEnumeratePhysicalDevices enumerate =
    (PFN_vkEnumeratePhysicalDevices)instance_function(
      context, "vkEnumeratePhysicalDevices"
    );
  const PFN_vkGetPhysicalDeviceQueueFamilyProperties get_families =
    (PFN_vkGetPhysicalDeviceQueueFamilyProperties)instance_function(
      context, "vkGetPhysicalDeviceQueueFamilyProperties"
    );
  TEST_ASSERT_NOT_NULL(enumerate);
  TEST_ASSERT_NOT_NULL(get_families);
  gpu_list list = {.count = max_gpus};
  MLN_TEST_STATUS(
    VK_SUCCESS, enumerate((VkInstance)context->instance, &list.count, list.gpus)
  );
  for (uint32_t index = 0; index < list.count; index += 1) {
    physical_device_uuid(context, list.gpus[index], list.uuids[index]);
    VkQueueFamilyProperties families[16];
    uint32_t family_count = sizeof(families) / sizeof(families[0]);
    get_families(list.gpus[index], &family_count, families);
    for (uint32_t family = 0; family < family_count; family += 1) {
      list.graphics[index] |=
        (families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
        families[family].queueCount > 0;
    }
  }
  return list;
}

static mln_vulkan_device_options uuid_options(const uint8_t* uuid) {
  mln_vulkan_device_options options = {
    .size = sizeof(mln_vulkan_device_options)
  };
  options.fields = MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID;
  options.device_uuid = (mln_buffer_view){.data = uuid, .size = VK_UUID_SIZE};
  return options;
}

// Each GPU, whichever one the default would pick, and none for a UUID that no
// GPU has.
static void create_picks_the_gpu_that_device_uuid_names(void) {
  const mln_vulkan_device first = create_device(NULL);
  const mln_vulkan_context_descriptor context = device_context(first);
  const gpu_list list = list_gpus(&context);
  mln_vulkan_device_release(first);
  for (uint32_t index = 0; index < list.count; index += 1) {
    if (!list.graphics[index]) continue;
    const mln_vulkan_device_options options = uuid_options(list.uuids[index]);
    const mln_vulkan_device named = create_device(&options);
    const mln_vulkan_context_descriptor named_context = device_context(named);
    uint8_t picked[VK_UUID_SIZE];
    physical_device_uuid(&named_context, named_context.physical_device, picked);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(list.uuids[index], picked, VK_UUID_SIZE);
    mln_vulkan_device_release(named);
  }

  // Byte `index` differs from that byte of GPU `index`'s UUID, so it matches
  // no GPU.
  uint8_t unknown[VK_UUID_SIZE];
  for (uint32_t index = 0; index < VK_UUID_SIZE; index += 1) {
    unknown[index] = (uint8_t)~list.uuids[index % list.count][index];
  }
  const mln_vulkan_device_options options = uuid_options(unknown);
  mln_vulkan_device device = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_FOUND,
    mln_vulkan_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}

// A session keeps its device after the host releases it, so the renderer's
// objects on the device outlive the release.
static void a_released_device_lasts_until_its_session_detaches(void) {
  const mln_vulkan_device device = create_device(NULL);
  const mln_vulkan_context_descriptor context = device_context(device);
  mln_test_graphics* graphics = borrow_graphics(&context);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_vulkan_on(map, &fixture, graphics),
    mln_test_graphics_last_error()
  );
  render_one_frame(&fixture, 1);

  mln_vulkan_device_release(device);
  render_one_frame(&fixture, 2);
  // The host's own objects on the device go before the session does.
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_release_target(
      fixture.session, &completion.descriptor, NULL
    )
  );
  mln_test_render_fixture_destroy_textures(&fixture);
  mln_test_graphics_destroy(graphics);
  detach(&fixture);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(create_rejects_malformed_options);
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(create_reports_an_unsupported_backend);
#else
  RUN_TEST(a_session_renders_with_the_device_context);
  RUN_TEST(create_picks_the_gpu_that_device_uuid_names);
  RUN_TEST(a_released_device_lasts_until_its_session_detaches);
#endif
}
